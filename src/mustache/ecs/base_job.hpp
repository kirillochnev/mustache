#pragma once

#include <mustache/ecs/world_filter.hpp>
#include <mustache/ecs/component_mask.hpp>

namespace mustache {
    class Archetype;
    class World;
    class Dispatcher;

    enum class JobRunMode : uint32_t {
        kCurrentThread = 0u,
        kParallel = 1u,
        kSingleThread = 2u,
        kDefault = kCurrentThread,
    };

    struct TasksCount : public IndexLike<uint32_t, TasksCount>{};
    struct JobSize : public IndexLike<uint32_t, JobSize>{};

    class Benchmark;

    class BaseJob {
    public:
        BaseJob();
        virtual ~BaseJob();

        void run(World& world, JobRunMode mode = JobRunMode::kDefault);

        virtual void runParallel(World&, uint32_t num_tasks) = 0;
        virtual void runCurrentThread(World&) = 0;
        virtual ComponentIdMask checkMask() const noexcept = 0;
        virtual ComponentIdMask updateMask() const noexcept = 0;
        [[nodiscard]] virtual std::string name() const noexcept = 0;

        [[nodiscard]] virtual bool extraArchetypeFilterCheck(const Archetype&) const noexcept {
            return true;
        }
        [[nodiscard]] virtual bool extraChunkFilterCheck(const Archetype&, ChunkIndex) const noexcept {
            return true;
        }

        virtual uint32_t applyFilter(World&) noexcept;
        [[nodiscard]] virtual uint32_t taskCount(World&, uint32_t entity_count) const noexcept;

        virtual void onJobBegin(World&, TasksCount, JobSize total_entity_count, JobRunMode mode) noexcept;
        virtual void onJobEnd(World&, TasksCount, JobSize total_entity_count, JobRunMode mode) noexcept;

        void enableBenchmark();
        void disableBenchmark();
        void showBenchmark();
    protected:
        WorldVersion last_update_version_;
        WorldFilterResult filter_result_;
        std::string name_;
        std::shared_ptr<Benchmark> benchmark_;
    };
}
