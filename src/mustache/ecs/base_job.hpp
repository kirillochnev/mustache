#pragma once

#include <mustache/ecs/component_mask.hpp>

namespace mustache {

    class World;
    class Dispatcher;


    enum class JobRunMode : uint32_t {
        kCurrentThread = 0u,
        kParallel = 1u,
        kSingleThread = 2u,
        kDefault = kCurrentThread,
    };

    class BaseJob {
    public:
        virtual ~BaseJob() = default;

        void run(World& world, JobRunMode mode = JobRunMode::kDefault);

        virtual void runParallel(World&, uint32_t num_tasks) = 0;
        virtual void runCurrentThread(World&) = 0;
        virtual ComponentIdMask checkMask() const noexcept = 0;
        virtual ComponentIdMask updateMask() const noexcept = 0;
        virtual uint32_t applyFilter(World&) noexcept = 0;
        [[nodiscard]] virtual uint32_t taskCount(World&, uint32_t entity_count) const noexcept;

    };
}
