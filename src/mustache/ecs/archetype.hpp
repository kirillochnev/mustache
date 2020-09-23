#pragma once

#include <mustache/ecs/chunk.hpp>
#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache/ecs/archetype_operation_helper.hpp>
#include <mustache/ecs/component_data_storage.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/entity_group.hpp>
#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/type_info.hpp>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace mustache {

    class World;
    class EntityManager;

    // TODO: only Archetype can use
    struct ArchetypeInternalEntityLocation {
        Chunk* chunk;
        ChunkEntityIndex index;
    };

    /**
     * Stores Entities with same component set
     * NOTE: It is has no information about entity manager, so Archetype's methods don't effects entity location.
     */
    class Archetype : public Uncopiable {
    public:
        Archetype(World& world, ArchetypeIndex id, const ComponentMask& mask);
        ~Archetype();

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        Entity entityAt(ArchetypeEntityIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (!index.isValid() || index.toInt() >= data_storage_.size()) {
                    return Entity{};
                }
            }
            auto entity_ptr = data_storage_.getEntityData<_Safety>(ComponentStorageIndex::fromArchetypeIndex(index));
            return entity_ptr ? *entity_ptr : Entity{};
        }

        [[nodiscard]] uint32_t chunkCapacity() const noexcept {
            return data_storage_.chunk_capacity_.toInt();
        }

        [[nodiscard]] EntityGroup createGroup(size_t count);

        [[nodiscard]] uint32_t size() const noexcept {
            return data_storage_.size();
        }
        [[nodiscard]] uint32_t capacity() const noexcept {
            return data_storage_.capacity();
        }
        [[nodiscard]] ArchetypeIndex id() const noexcept {
            return id_;
        }

        bool isMatch(const ComponentMask& mask) const noexcept {
            return mask_.isMatch(mask);
        }

        bool hasComponent(ComponentId component_id) const noexcept {
            return mask_.has(component_id);
        }
        template<typename T>
        bool hasComponent() const noexcept {
            static const auto component_id = ComponentFactory::registerComponent<T>();
            return mask_.has(component_id);
        }
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        Chunk* getChunk(ChunkIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (!index.isValid() || ! data_storage_.chunks_.has(index)) {
                    return nullptr;
                }
            }
            return data_storage_.chunks_[index];
        }
        size_t chunkCount() const noexcept {
            return data_storage_.chunks_.size();
        }

        WorldVersion worldVersion() const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] ComponentIndex getComponentIndex(ComponentId id) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (!operation_helper_.component_id_to_component_index.has(id)) {
                    return ComponentIndex::null();
                }
            }
            return operation_helper_.component_id_to_component_index[id];
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] WorldVersion getWorldVersionComponentUpdate(ComponentId id,
                ArchetypeEntityIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (!hasComponent(id)) {
                    return WorldVersion::null();
                }
            }
            return data_storage_.getVersion(getComponentIndex(id), ComponentStorageIndex::fromArchetypeIndex(index));
        }

        [[nodiscard]] const ComponentMask& componentMask() const noexcept {
            return mask_;
        }

        template<typename T, FunctionSafety _Safety = FunctionSafety::kDefault>
        T* getComponent(const ArchetypeInternalEntityLocation& location) const noexcept {
            const auto component_id = ComponentFactory::registerComponent<T>();
            const auto component_index = operation_helper_.componentIndex<_Safety>(component_id);
            return static_cast<T*>(data_storage_.getData<_Safety>(component_index, location.chunk, location.index));
        }

        [[nodiscard]] auto getElementView(ArchetypeEntityIndex index) const noexcept {
            return data_storage_.getElementView(ComponentStorageIndex::fromArchetypeIndex(index));
        }
    private:
        friend class EntityManager;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        ArchetypeInternalEntityLocation entityIndexToInternalLocation(ArchetypeEntityIndex index) const noexcept {
            ArchetypeInternalEntityLocation result {
                    nullptr,
                    ChunkEntityIndex::null()
            };
            if constexpr (isSafe(_Safety)) {
                if (index.toInt() >= data_storage_.size()) {
                    return result;
                }
            }
            const auto chunk_capacity = data_storage_.chunkCapacity();
            const auto storage_index = ComponentStorageIndex::fromArchetypeIndex(index);
            result.chunk = data_storage_.chunks_[storage_index / chunk_capacity];
            result.index = storage_index % chunk_capacity;
            return result;
        }

        template<typename _F>
        void forEachEntity(_F&& callback) {
            if (data_storage_.isEmpty()) {
                return;
            }
            auto view = data_storage_.getElementView(ComponentStorageIndex::make(0));
            while (view.isValid()) {
                callback(*view.getEntity<FunctionSafety::kUnsafe>());
                ++view;
            }
        }

        void clear();

        /// Entity must belong to default(empty) archetype
        ArchetypeEntityIndex insert(Entity entity, bool call_constructor = true);

        ArchetypeEntityIndex insert(Entity entity, Archetype& prev_archetype,
                ArchetypeEntityIndex prev_index, bool initialize_missing_components = true);

        /**
         * removes entity from archetype, calls destructor for each trivially destructible component
         * moves last entity at index.
         * returns new entity at index.
         */
        Entity remove(ArchetypeEntityIndex index);

        World& world_;
        ComponentMask mask_;
        ArchetypeOperationHelper operation_helper_;
        ComponentDataStorage data_storage_;

        ArchetypeIndex id_;
        std::string name_;
    };

}
