#pragma once

#include <mustache/utils/dll_export.h>
#include <mustache/utils/array_wrapper.hpp>

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


        struct CreateActionIndex : public IndexLike<size_t, CreateActionIndex> {};

        struct ActionInfo {
            ActionInfo() noexcept = default;
            ActionInfo(const Entity& e, Action a)  noexcept :
                entity{ e },
                action{ a } {

            }
            Entity entity;
            Action action;
            ComponentId component_id;
            const ComponentInfo* type_info = nullptr;


            std::byte* ptr = nullptr;
            CreateActionIndex create_action_index;
        };
        struct CreateAction {
            ComponentIdMask mask;
            SharedComponentsInfo shared;
        };
        ArrayWrapper<CreateAction, CreateActionIndex, false> create_actions_;
        std::vector<ActionInfo> actions_;

        ActionInfo& emplaceItem(Entity enity, Action action);

        std::byte* allocate(uint32_t size);

        std::vector<DataChunk> chunks_;
        uint32_t target_chunk_size_ = 4096u;
        uint32_t total_size_ = 0u;
    };
}
