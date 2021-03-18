#include "base_job.hpp"
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/world_filter.hpp>
#include <mustache/utils/benchmark.hpp>

using namespace mustache;

namespace {
    void filterArchetype(Archetype& archetype, const ArchetypeFilterParam& check, const ArchetypeFilterParam& set,
                         WorldFilterResult& result, BaseJob& job) {

        WorldFilterResult::ArchetypeFilterResult item;
        item.archetype = &archetype;
        item.entities_count = 0u;

        const auto last_index = archetype.lastChunkIndex();
        bool is_prev_match = false;
        WorldFilterResult::EntityBlock block{ArchetypeEntityIndex::make(0), ArchetypeEntityIndex::make(0)};
        const auto chunk_size = archetype.chunkCapacity().toInt();
        for (auto chunk_index = ChunkIndex::make(0); chunk_index <= last_index; ++chunk_index) {
            const bool is_match =
                    job.extraChunkFilterCheck(archetype, chunk_index) &&
                    archetype.versionStorage().checkAndSet(check, set, chunk_index);

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
                archetype_check.mask = arch.makeComponentMask(check.mask).items();
                ArchetypeFilterParam archetype_set;
                archetype_set.version = set.version;
                archetype_set.mask = arch.makeComponentMask(set.mask).items();
                if (arch.versionStorage().checkAndSet(archetype_check, archetype_set)) {
                    filterArchetype(arch, archetype_check, archetype_set, result, job);
                }
            }
        }

        return result.total_entity_count > 0u;
    }
}
BaseJob::BaseJob() {
    enableBenchmark();
}

BaseJob::~BaseJob() {
    if (!benchmark_) {
        return;
    }
    std::cout << "-------------  " << name_ << "-------------  " << "\n";
    benchmark_->show();
}

uint32_t BaseJob::taskCount(World& world, uint32_t entity_count) const noexcept {
    return std::min(entity_count, world.dispatcher().threadCount() + 1);
}

void BaseJob::run(World& world, JobRunMode mode) {
    name_ = name();
    const auto entities_count = applyFilter(world);
    if (entities_count < 1u) {
        return;
    }

    const auto task_count = (mode == JobRunMode::kParallel) ? std::max(1u, taskCount(world, entities_count)) : 1u;
    if (task_count > 0u) {
        world.incrementVersion();

        onJobBegin(world, TasksCount::make(task_count), JobSize::make(entities_count), mode);
        const auto do_job = [this, mode, &world, task_count] {
            if (mode == JobRunMode::kCurrentThread) {
                runCurrentThread(world);
            } else {
                runParallel(world, task_count);
            }
        };
        if (benchmark_) {
            benchmark_->add(do_job);
        } else {
            do_job();
        }
        onJobEnd(world, TasksCount::make(task_count), JobSize::make(entities_count), mode);
    }
}

uint32_t BaseJob::applyFilter(World& world) noexcept {
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

void BaseJob::enableBenchmark() {
    if (!benchmark_) {
        benchmark_ = std::make_shared<Benchmark>();
    }
}

void BaseJob::disableBenchmark() {
    benchmark_.reset();
}

void BaseJob::showBenchmark() {
    /*if (!benchmark_) {
        return;
    }
    std::cout << "-------------  " << name_ << "-------------  " << "\n";
    benchmark_->show();*/
}
