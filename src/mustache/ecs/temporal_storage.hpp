#pragma once

#include <mustache/utils/dll_export.h>

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <vector>

namespace mustache {
    class Archetype;
    struct ComponentInfo;

    struct MUSTACHE_EXPORT TemporalStorage {
        enum class Action : uint32_t {
            kDestroyEntityNow = 0,
            kCreateEntity = 1,
            kDestroyEntity = 2,
            kRemoveComponent = 3,
            kAssignComponent = 4,
        };
        TemporalStorage() = default;
        TemporalStorage(TemporalStorage&&) = default;
        ~TemporalStorage() {
            clear();
        }
        void* assignComponent(World& world, Entity entity, ComponentId id, bool skip_constructor);

        void create(Entity entity, Archetype* archetype);

        void removeComponent(Entity entity, ComponentId id);

        // destroy after all systems update
        void destroy(Entity entity);
        // destroy after EntityManager::unlock()
        void destroyNow(Entity entity);

        void clear();

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
            const ComponentInfo* type_info;
            std::byte* ptr;
        };

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

        std::byte* allocate(uint32_t size);

        std::vector<ActionInfo> actions_;
        struct {
            std::vector<AssignComponentWithArgs> assign;
            std::vector<RemoveComponent> remove;
            std::vector<Archetype*> create;
        } commands_;
        std::vector<DataChunk> chunks_;
        uint32_t target_chunk_size_ = 4096u;
        uint32_t total_size_ = 0u;
    };
}
