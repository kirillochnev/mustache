#include "world.hpp"

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

}

World::~World() {
    used_world_ids.erase(id_);
}

void World::update() {
    ++version_;

    if (systems_) {
        systems_->update();
    }

    entities().update();
}

WorldId World::nextWorldId() noexcept {
    if (!used_world_ids.empty()) {
        auto result = *used_world_ids.begin();
        used_world_ids.erase(result);
        return result;
    }
    auto result = next_id;
    ++next_id;
    return result;
}
