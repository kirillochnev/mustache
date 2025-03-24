#include "base_job.hpp"

#include <mustache/utils/profiler.hpp>

#include <mustache/ecs/world.hpp>
#include <mustache/ecs/world_filter.hpp>

using namespace mustache;

namespace {
    void filterArchetype(Archetype& archetype, const ArchetypeFilterParam& check, const ArchetypeFilterParam& set,
                         WorldFilterResult& result, BaseJob& job) {
        MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );
        WorldFilterResult::ArchetypeFilterResult item;
        item.archetype = &archetype;
        item.entities_count = 0u;

        const auto last_index = archetype.lastChunkIndex();
        bool is_prev_match = false;
        WorldFilterResult::EntityBlock block{ArchetypeEntityIndex::make(0), ArchetypeEntityIndex::make(0)};
        const auto chunk_size = archetype.chunkCapacity().toInt();

        // TODO: make 2-3 levels of component version (for group of 1024, 32, 1) and let VersionStorage return filtered range
        for (auto chunk_index = ChunkIndex::make(0); chunk_index <= last_index; ++chunk_index) {
            const bool is_match =
                    job.extraChunkFilterCheck(archetype, chunk_index) &&
                    archetype.checkAndSet(check, set, chunk_index);

            if (is_match) {
                if (!is_prev_match) {
                    block.begin = ArchetypeEntityIndex::make(chunk_index.toInt() * chunk_size);
                }
                block.end = ArchetypeEntityIndex::make(chunk_index.next().toInt() * chunk_size);
            } else {
                if (is_prev_match) {
                    item.addBlock(block);
                }
            }
            is_prev_match = is_match;
        }
        if (is_prev_match) {
            const auto end_of_archetype = ArchetypeEntityIndex::make(archetype.size());
            block.end = std::min(end_of_archetype, block.end);
            item.addBlock(block);
        }
        if (item.entities_count > 0) {
            result.filtered_archetypes.push_back(item);
            result.total_entity_count += item.entities_count;
        }
    }

    bool apply(World& world, const FilterCheckParam& check, const FilterSetParam& set,
               WorldFilterResult& result, BaseJob& job) {
        MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

        auto& entities = world.entities();
        const auto num_archetypes = entities.getArchetypesCount();
        result.filtered_archetypes.reserve(num_archetypes);

        ArchetypeFilterParam archetype_check;
        archetype_check.version = check.version;
        for (ArchetypeIndex index = ArchetypeIndex::make(0); index < ArchetypeIndex::make(num_archetypes); ++index) {
            auto& arch = entities.getArchetype(index);

            const bool is_archetype_match =
                    arch.size() > 0u &&
                    arch.isMatch(result.mask) &&
                    arch.isMatch(result.shared_component_mask) &&
                    job.extraArchetypeFilterCheck(arch);

            if (is_archetype_match) {
                archetype_check.mask = arch.makeComponentVersionControlEnabledMask(check.mask).items();
                ArchetypeFilterParam archetype_set;
                archetype_set.version = set.version;
                archetype_set.mask = arch.makeComponentVersionControlEnabledMask(set.mask).items();
                if (arch.checkAndSet(archetype_check, archetype_set)) {
                    filterArchetype(arch, archetype_check, archetype_set, result, job);
                }
            }
        }

        return result.total_entity_count > 0u;
    }
}

TasksCount BaseJob::taskCount(World& world, uint32_t entity_count) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );
    return TasksCount::make(std::min(entity_count, world.dispatcher().threadCount() + 1));
}

void BaseJob::run(World& world, JobRunMode mode) {
    MUSTACHE_PROFILER_BLOCK_LVL_0(nameCStr());
    const auto entities_count = applyFilter(world);
    if (entities_count < 1u) {
        return;
    }

    TasksCount task_count = TasksCount::make(1);
    if (mode == JobRunMode::kParallel) {
        task_count = std::max(TasksCount::make(1u), taskCount(world, entities_count));
    }

    if (task_count.toInt() > 0u) {
        world.incrementVersion();

        onJobBegin(world, task_count, JobSize::make(entities_count), mode);
        world.entities().lock();
        if (mode == JobRunMode::kCurrentThread) {
            runCurrentThread(world);
        } else {
            runParallel(world, task_count);
        }
        world.entities().unlock();
        onJobEnd(world, task_count, JobSize::make(entities_count), mode);
    }
}

uint32_t BaseJob::applyFilter(World& world) noexcept {
    // TODO: make this fast for small jobs
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    filter_result_.clear();

    const auto cur_world_version = world.version();
    const FilterCheckParam check {
            checkMask(),
            last_update_version_
    };
    const FilterSetParam set {
            updateMask(),
            cur_world_version
    };

    if (apply(world, check, set, filter_result_, *this)) {
        last_update_version_ = cur_world_version;
    }

    return filter_result_.total_entity_count;
}

void BaseJob::onJobBegin(World&, TasksCount, JobSize, JobRunMode) noexcept {

}

void BaseJob::onJobEnd(World&, TasksCount, JobSize, JobRunMode) noexcept {

}

void BaseJob::onTaskBegin(World&, TaskSize, ParallelTaskId) noexcept {

}

void BaseJob::onTaskEnd(World&, TaskSize, ParallelTaskId) noexcept {

}

void BaseJob::runParallel(World& world, TasksCount task_count) {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    auto& dispatcher = world.dispatcher();
    JobInvocationIndex invocation_index;
    invocation_index.entity_index = ParallelTaskGlobalItemIndex::make(0);
    invocation_index.entity_index_in_task = ParallelTaskItemIndexInTask::make(0);
    invocation_index.task_index = ParallelTaskId::make(0);

    for (ArchetypeGroup task : TaskGroup::make(filter_result_, task_count)) {
        dispatcher.addParallelTask([task, this, invocation_index, &world](ThreadId thread_id) mutable {
            invocation_index.thread_id = thread_id;
            const auto task_size = TaskSize::make(task.taskSize());
            {
                MUSTACHE_PROFILER_BLOCK_LVL_0("onTaskBegin");
                onTaskBegin(world, task_size, invocation_index.task_index);
            }
            {
                MUSTACHE_PROFILER_BLOCK_LVL_0("singleTask");
                singleTask(world, task, invocation_index);
            }
            MUSTACHE_PROFILER_BLOCK_LVL_0("onTaskEnd");
            onTaskEnd(world, task_size, invocation_index.task_index);
        });
        ++invocation_index.task_index;
        invocation_index.entity_index = ParallelTaskGlobalItemIndex::make(invocation_index.entity_index.toInt() + task.taskSize());
    }
    dispatcher.waitForParallelFinish();
}

void BaseJob::runCurrentThread(World& world) {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );
    auto& dispatcher = world.dispatcher();
    JobInvocationIndex invocation_index;
    invocation_index.task_index = ParallelTaskId::make(0);
    invocation_index.thread_id = dispatcher.currentThreadId();
    invocation_index.entity_index_in_task = ParallelTaskItemIndexInTask::make(0);
    invocation_index.entity_index = ParallelTaskGlobalItemIndex::make(0);

    for (auto task : TaskGroup::make(filter_result_, TasksCount::make(1))) {
        const auto task_size = TaskSize::make(task.taskSize());
        {
            MUSTACHE_PROFILER_BLOCK_LVL_0("onTaskBegin");
            onTaskBegin(world, task_size, invocation_index.task_index);
        }
        {
            MUSTACHE_PROFILER_BLOCK_LVL_0("singleTask");
            singleTask(world, task, invocation_index);
        }

        MUSTACHE_PROFILER_BLOCK_LVL_0("onTaskEnd");
        onTaskEnd(world, task_size, invocation_index.task_index);
        ++invocation_index.task_index;
    }
}
