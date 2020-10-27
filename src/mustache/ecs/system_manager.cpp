#include "system_manager.hpp"
#include <mustache/ecs/system.hpp>

#include <vector>

using namespace mustache;

struct SystemManager::Data {
    World& world;
    Data(World& w):
        world{w} {

    }

    std::vector<SystemPtr> systems;
};

SystemManager::SystemManager(World& world):
    data_{new Data{world} } {
}

SystemManager::~SystemManager() {
    for (auto& system : data_->systems) {
        system->destroy(data_->world);
    }
}

void SystemManager::update(World& world) {
    for (auto& system : data_->systems) {
        if (system->state() == SystemState::kActive) {
            system->update(world);
        }
    }
}

void mustache::SystemManager::addSystem(SystemManager::SystemPtr system) {
    data_->systems.emplace_back(std::move(system));
}
