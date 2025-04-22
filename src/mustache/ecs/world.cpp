#include "world.hpp"

#include <mustache/utils/profiler.hpp>

using namespace mustache;

namespace {
    mustache::set<WorldId> used_world_ids;
    WorldId next_id = WorldId::make(0);
}

World::World(const WorldContext& context, WorldId id):
    id_{id},
    context_{context},
    entities_{*this},
    world_storage_{memoryManager()} {
    MUSTACHE_PROFILER_BLOCK_LVL_0("World::World()");
}

World::~World() {
    MUSTACHE_PROFILER_BLOCK_LVL_0("World::~World()");
    used_world_ids.erase(id_);
}

void World::init() {
    MUSTACHE_PROFILER_BLOCK_LVL_0("World::init()");

    version_ = WorldVersion::make(0);

    if (systems_) {
        systems_->init();
    }
}

void World::update() {
    MUSTACHE_PROFILER_BLOCK_LVL_0("World::update()");

    incrementVersion();

    if (systems_) {
        systems_->init();
        systems_->update();
    }

    entities().update();
}

WorldId World::nextWorldId() noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3("World::nextWorldId()");

    if (!used_world_ids.empty()) {
        auto result = *used_world_ids.begin();
        used_world_ids.erase(result);
        return result;
    }
    auto result = next_id;
    ++next_id;
    return result;
}
