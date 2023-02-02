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
        ~TemporalStorage();


        void* assignComponent(World& world, Entity entity, ComponentId id, bool skip_constructor);

        void create(Entity entity, const ComponentIdMask& mask, const SharedComponentsInfo& shared);

        void removeComponent(Entity entity, ComponentId id);

        // destroy after all systems update
        void destroy(Entity entity);
        // destroy after EntityManager::unlock()
        void destroyNow(Entity entity);

        void clear();

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

        struct ActionInfo {
            ActionInfo() = default;
            ActionInfo(const Entity& e, Action a) :
                entity{ e },
                action{ a } {

            }
            Entity entity;
            Action action;
            ComponentId component_id;
            const ComponentInfo* type_info = nullptr;


            std::byte* ptr = nullptr;
            int32_t create_action_index = -1;
        };
        struct CreateAction {
            ComponentIdMask mask;
            SharedComponentsInfo shared;
        };
        std::vector<CreateAction> create_actions_;
        std::vector<ActionInfo> actions_;

        ActionInfo& emplaceItem(Entity enity, Action action);

        std::byte* allocate(uint32_t size);

        std::vector<DataChunk> chunks_;
        uint32_t target_chunk_size_ = 4096u;
        uint32_t total_size_ = 0u;
    };
}
