#include "world.hpp"

#include <mustache/utils/profiler.hpp>

using namespace mustache;

namespace {
    std::set<WorldId> used_world_ids;
    WorldId next_id = WorldId::make(0);
}

World::World(const WorldContext& context, WorldId id):
    id_{id},
    context_{context},
    entities_{*this},
    world_storage_{*context.memory_manager} {
    MUSTACHE_PROFILER_BLOCK_LVL_0(("World() id:" + std::to_string(id_.toInt())).c_str());
}

World::~World() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(("~World() id:" + std::to_string(id_.toInt())).c_str());
    used_world_ids.erase(id_);
}

void World::init() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    version_ = WorldVersion::make(0);

    if (systems_) {
        systems_->init();
    }
}

void World::update() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );
    incrementVersion();

    if (systems_) {
        systems_->update();
    }

    entities().update();
}

WorldId World::nextWorldId() noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );

    if (!used_world_ids.empty()) {
        auto result = *used_world_ids.begin();
        used_world_ids.erase(result);
        return result;
    }
    auto result = next_id;
    ++next_id;
    return result;
}
