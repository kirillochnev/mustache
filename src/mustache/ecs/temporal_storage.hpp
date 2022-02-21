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
        TemporalStorage() = default;
        TemporalStorage(TemporalStorage&&) = default;
        ~TemporalStorage() {
            clear();
        }
        void* assignComponent(Entity entity, ComponentId id, bool skip_constructor) {
            const auto& component_info = ComponentFactory::componentInfo(id);
            auto& info = actions_.emplace_back();
            info.action = Action::kAssignComponent;
            info.entity = entity;
            info.index = static_cast<uint32_t >(commands_.assign.size());
            auto& command = commands_.assign.emplace_back();
            command.type_info = &component_info;
            command.component_id = id;
            command.ptr = allocate(static_cast<uint32_t >(component_info.size));
            if (!skip_constructor && component_info.functions.create) {
                component_info.functions.create(command.ptr);
            }
            return command.ptr;
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
            for (const auto& assign_action : commands_.assign) {
                if (assign_action.type_info != nullptr && assign_action.type_info->functions.destroy != nullptr) {
                    assign_action.type_info->functions.destroy(assign_action.ptr);
                }
            }
            actions_.clear();
            commands_.assign.clear();
            commands_.remove.clear();
            if (chunks_.size() > 1u) {
                chunks_.clear();
            } else {
                if (!chunks_.empty()) {
                    chunks_.front().free_space = chunks_.front().capacity;
                }
            }
            target_chunk_size_ = total_size_;
            total_size_ = 0u;
        }

        struct ActionInfo {
            Entity entity;
            Action action;
            uint32_t index;
        };
        struct RemoveComponent {
            ComponentId component_id;
        };

        struct AssignComponentWithArgs {
            ComponentId component_id;
            const TypeInfo* type_info;
            std::byte* ptr;
        };

        std::vector<ActionInfo> actions_;
        struct {
            std::vector<AssignComponentWithArgs> assign;
            std::vector<RemoveComponent> remove;
        } commands_;

        struct DataChunk {
            DataChunk(uint32_t size):
                free_space{size},
                capacity{size},
                data{new std::byte[size]} {

            }
            DataChunk(DataChunk&&) = default;

            uint32_t free_space;
            uint32_t capacity;
            std::unique_ptr<std::byte[]> data;
        };

        std::byte* allocate(uint32_t size) {
            if (chunks_.empty() || chunks_.back().free_space < size) {
                if (target_chunk_size_ < size) {
                    target_chunk_size_ = size;
                }
                chunks_.emplace_back(target_chunk_size_);
            }
            auto& chunk = chunks_.back();
            const auto offset = chunk.capacity - chunk.free_space;
            chunk.free_space -= size;
            total_size_ += size;
            return chunk.data.get() + offset;
        }

        std::vector<DataChunk> chunks_;
        uint32_t target_chunk_size_ = 4096u;
        uint32_t total_size_ = 0u;
    };
}
