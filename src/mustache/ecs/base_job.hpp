#pragma once

#include <mustache/utils/dispatch.hpp>

#include <mustache/ecs/task_view.hpp>
#include <mustache/ecs/world_filter.hpp>
#include <mustache/ecs/component_mask.hpp>

namespace mustache {
    class Archetype;
    class World;
    class Dispatcher;

    enum class JobUnroll : uint32_t {
        kAuto = 0,
        kEnabled,
        kDisabled
    };
    enum class JobRunMode : uint32_t {
        kCurrentThread = 0u,
        kParallel = 1u,
        kSingleThread = 2u,
        kDefault = kCurrentThread,
    };

    class MUSTACHE_EXPORT BaseJob {
    public:
        virtual ~BaseJob() = default;

        void run(World& world, JobRunMode mode = JobRunMode::kDefault);

        virtual void runParallel(World&, TasksCount num_tasks);
        virtual void runCurrentThread(World&);
        virtual void singleTask(World& world, ArchetypeGroup archetype_group,
                                        JobInvocationIndex invocation_index) = 0;
        virtual ComponentIdMask checkMask() const noexcept = 0;
        virtual ComponentIdMask updateMask() const noexcept = 0;
        [[nodiscard]] virtual std::string name() const noexcept {
            return nameCStr();
        }
        [[nodiscard]] virtual const char* nameCStr() const noexcept = 0;

        [[nodiscard]] virtual bool extraArchetypeFilterCheck(const Archetype&) const noexcept {
            return true;
        }
        [[nodiscard]] virtual bool extraChunkFilterCheck(const Archetype&, ChunkIndex) const noexcept {
            return true;
        }

        virtual uint32_t applyFilter(World&) noexcept;
        [[nodiscard]] virtual TasksCount taskCount(World&, uint32_t entity_count) const noexcept;
        virtual void onTaskBegin(World&, TaskSize size, ParallelTaskId task_id) noexcept;
        virtual void onTaskEnd(World&, TaskSize size, ParallelTaskId task_id) noexcept;

        virtual void onJobBegin(World&, TasksCount, JobSize total_entity_count, JobRunMode mode) noexcept;
        virtual void onJobEnd(World&, TasksCount, JobSize total_entity_count, JobRunMode mode) noexcept;

    protected:
        WorldVersion last_update_version_;
        WorldFilterResult filter_result_;
    };
}
