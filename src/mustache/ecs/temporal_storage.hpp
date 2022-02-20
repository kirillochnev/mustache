#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <vector>

namespace mustache {

    struct TemporalStorage {
        enum class Action : uint32_t {
            kDestroyEntityNow = 0,
            kDestroyEntity = 1,
            kRemoveComponent = 2,
            kAssignComponent = 3,
        };

        void* assignComponent(Entity entity, ComponentId id, bool skip_constructor) {
            const auto& component_info = ComponentFactory::componentInfo(id);
            auto& info = actions_.emplace_back();
            info.action = Action::kAssignComponent;
            info.entity = entity;
            info.index = static_cast<uint32_t >(commands_.assign.size());
            auto& command = commands_.assign.emplace_back();
            command.component_id = id;
            command.data_offset = static_cast<uint32_t >(data_.size());
            data_.resize(command.data_offset + component_info.size);
            auto ptr = data_.data() + command.data_offset;
            if (!skip_constructor && component_info.functions.create) {
                component_info.functions.create(ptr);
            }
            return ptr;
        }

        void removeComponent(Entity entity, ComponentId id) {
            auto& info = actions_.emplace_back();
            info.action = Action::kRemoveComponent;
            info.entity = entity;
            info.index = static_cast<uint32_t >(commands_.remove.size());
            commands_.remove.emplace_back().component_id = id;
        }

        // destroy after all systems update
        void destroy(Entity entity) {
            auto& info = actions_.emplace_back();
            info.action = Action::kDestroyEntity;
            info.entity = entity;
        }
        // destroy after EntityManager::unlock()
        void destroyNow(Entity entity) {
            auto& info = actions_.emplace_back();
            info.action = Action::kDestroyEntityNow;
            info.entity = entity;
        }

        void clear() {
            actions_.clear();
            commands_.assign.clear();
            commands_.remove.clear();
            data_.clear();
        }

        struct ActionInfo {
            Entity entity;
            Action action;
            uint32_t index;
        };
        struct AssignOrRemoveComponent {
            ComponentId component_id;
        };

        struct AssignComponentWithArgs {
            ComponentId component_id;
            uint32_t data_offset;
        };

        std::vector<ActionInfo> actions_;
        struct {
            std::vector<AssignComponentWithArgs> assign;
            std::vector<AssignOrRemoveComponent> remove;
        } commands_;
        std::vector<std::byte> data_;
    };
}
