#include "base_job.hpp"
#include <mustache/ecs/world.hpp>

using namespace mustache;

uint32_t BaseJob::taskCount(World& world, uint32_t entity_count) const noexcept {
    return std::min(entity_count, world.dispatcher().threadCount() + 1);
}

void BaseJob::run(World& world, JobRunMode mode) {
    const auto entities_count = applyFilter(world);
    switch (mode) {
        case JobRunMode::kCurrentThread:
            runCurrentThread(world);
            break;
        case JobRunMode::kParallel:
            runParallel(world, taskCount(world, entities_count));
            break;
        case JobRunMode::kSingleThread:
            runParallel(world, 1);
            break;
    };
}
