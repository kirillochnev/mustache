#include "system_manager.hpp"

#include <mustache/ecs/system.hpp>

#include <map>
#include <vector>
#include <algorithm>
#include <stdexcept>

using namespace mustache;

struct SystemManager::Data {
    World& world;

    Data(World& w) :
            world{w} {

    }

    bool was_init = false;
    struct SystemInfo {
        SystemPtr system;
        SystemConfig config;
        bool operator==(const SystemPtr& rhs) const noexcept {
            return system == rhs;
        }
    };
    std::vector<SystemInfo> systems_info;
    std::vector<SystemPtr> ordered_systems;
    std::map<std::string, int32_t> group_priorities;
    std::map<std::string, SystemPtr > system_by_name;
};

SystemManager::SystemManager(World& world) :
        data_{new Data{world}} {
}

SystemManager::~SystemManager() {
    for (const auto& system : data_->ordered_systems) {
        system->destroy(data_->world);
    }
}

void SystemManager::update() {
    if (!data_->was_init) {
        return;
    }

    for (const auto& system : data_->ordered_systems) {
        auto state = system->state();

        if (system->state() == SystemState::kConfigured) {
            system->start(data_->world);
            state = system->state();
        }

        if (state == SystemState::kActive) {
            system->update(data_->world);
        }
    }
}

void mustache::SystemManager::init() {
    if (data_->was_init) {
        return;
    }
    data_->was_init = true;
    for (auto& info : data_->systems_info) {
        auto state = info.system->state();
        if (state == SystemState::kInited) {
            info.system->configure(data_->world, info.config);
        }
    }

    reorderSystems();

    for (const auto& system : data_->ordered_systems) {
        if (system->state() == SystemState::kConfigured) {
            system->start(data_->world);
        }
    }
}

void mustache::SystemManager::addSystem(SystemManager::SystemPtr system) {
    const auto index = data_->systems_info.size();
    data_->systems_info.emplace_back();

    data_->systems_info[index].system = std::move(system);

    if (data_->systems_info[index].system->state() == SystemState::kUninit) {
        data_->systems_info[index].system->create(data_->world);
    }

    data_->system_by_name[data_->systems_info[index].system->name()] = data_->systems_info[index].system;

    if (data_->was_init) {
        data_->systems_info[index].system->configure(data_->world, data_->systems_info[index].config);
        reorderSystems();
    }
}

void mustache::SystemManager::reorderSystems() {
    auto systems_cpy = data_->systems_info;
    std::set<std::string> unplaced_systems_names;
    {
        std::map<std::string, Data::SystemInfo*> map;
        for (auto& info : data_->systems_info) {
            const auto name = info.system->name();
            unplaced_systems_names.insert(name);
            map[name] = &info;
        }

        for (const auto& pair : map) {
            for (const auto& sys_name : pair.second->config.update_before) {
                const auto find_res = map.find(sys_name);
                if (find_res != map.end()) {
                    find_res->second->config.update_after.insert(pair.first);
                }
            }
        }
    }

    std::sort(systems_cpy.begin(), systems_cpy.end(),
              [this](const Data::SystemInfo& a, const Data::SystemInfo& b) {
                  const auto group_prior_a = getGroupPriority(a.config.update_group);
                  const auto group_prior_b = getGroupPriority(b.config.update_group);
                  if (group_prior_a != group_prior_b) {
                      return group_prior_a > group_prior_b;
                  }
                  return a.config.priority > b.config.priority;
              });

    std::vector<SystemPtr> ordered_systems;
    ordered_systems.reserve(systems_cpy.size());

    const auto& can_be_placed = [&unplaced_systems_names](const Data::SystemInfo& info) {
        for (const auto& depend_on : info.config.update_after) {
            if (unplaced_systems_names.count(depend_on) > 0) {
                return false;
            }
        }
        return true;
    };

    while (!systems_cpy.empty()) {
        bool was_placement = false;
        for (uint32_t i = 0; i < systems_cpy.size(); ++i) {
            const auto& info = systems_cpy[i];
            if (can_be_placed(info)) {
                const auto name = info.system->name();
                ordered_systems.push_back(info.system);
                unplaced_systems_names.erase(name);
                systems_cpy.erase(systems_cpy.begin() + i);
                was_placement = true;
                break;
            }
        }
        if (!was_placement) {
            throw std::runtime_error("Can not reorder systems");
        }
    }

    data_->ordered_systems = ordered_systems;
}

int32_t mustache::SystemManager::getGroupPriority(const std::string& group_name) const noexcept {
    const auto find_res = data_->group_priorities.find(group_name);
    return find_res == data_->group_priorities.end() ? 0 : find_res->second;
}

void mustache::SystemManager::setGroupPriority(const std::string& group_name, int32_t priority) noexcept {
    data_->group_priorities[group_name] = priority;
}

SystemManager::SystemPtr mustache::SystemManager::findSystem(const std::string& system_name) noexcept {
    const auto find_res = data_->system_by_name.find(system_name);
    return find_res == data_->system_by_name.end() ? nullptr : find_res->second;
}

void mustache::SystemManager::removeSystem(const std::string& system_name) noexcept {
    const auto find_res = data_->system_by_name.find(system_name);
    if (find_res == data_->system_by_name.end()) {
        return;
    }

    (void ) std::remove(data_->ordered_systems.begin(), data_->ordered_systems.end(), find_res->second);
    (void ) std::remove(data_->systems_info.begin(), data_->systems_info.end(), find_res->second);
    data_->system_by_name.erase(find_res);

    reorderSystems();
}
