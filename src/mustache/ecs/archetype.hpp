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
    /// Location in world's entity manager
    struct EntityLocationInWorld { // TODO: add version here?
        constexpr static ArchetypeIndex kDefaultArchetype = ArchetypeIndex::null();
        EntityLocationInWorld() = default;
        EntityLocationInWorld(ArchetypeEntityIndex i, ArchetypeIndex arch ) noexcept :
            index{i},
            archetype{arch} {

        }
        ArchetypeEntityIndex index = ArchetypeEntityIndex::make(0u);
        ArchetypeIndex archetype = kDefaultArchetype;
    };

    template<typename T, bool _IsConst>
    struct ComponentPtrHelper {
        static constexpr auto getNullPtr() noexcept {
            if constexpr (_IsConst) {
                return static_cast<const T *>(nullptr);
            } else {
                return static_cast<T *>(nullptr);
            }
        }

        using type = decltype(getNullPtr());
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
            ArchetypeInternalEntityLocation location = entityIndexToInternalLocation(index);
            auto entity_ptr = operation_helper_.getEntity<_Safety>(location);
            return entity_ptr ? *entity_ptr : Entity{};
        }

        [[nodiscard]] uint32_t chunkCapacity() const noexcept {
            return data_storage_.chunk_capacity_;
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        Entity* getEntity(const ArchetypeInternalEntityLocation& location) const noexcept {
            return operation_helper_.getEntity<_Safety>(location);
        }

        void updateComponentsVersion(WorldVersion world_version, Chunk& chunk) const noexcept {
            return operation_helper_.updateComponentsVersion(world_version, chunk);
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
                if (!index.isValid() || !/*chunks_*/ data_storage_.chunks_.has(index)) {
                    return nullptr;
                }
            }
            return /*chunks_*/ data_storage_.chunks_[index];
        }
        size_t chunkCount() const noexcept {
            return /*chunks_*/ data_storage_.chunks_.size();
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

        [[nodiscard]] WorldVersion getWorldVersionComponentUpdate(ComponentIndex index,
                const ArchetypeInternalEntityLocation& location) const noexcept {
            return location.chunk->componentVersion(index);
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] WorldVersion getWorldVersionComponentUpdate(ComponentId id,
                ArchetypeEntityIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (!hasComponent(id)) {
                    return WorldVersion::null();
                }
            }
            return getWorldVersionComponentUpdate(getComponentIndex(id),
                    entityIndexToInternalLocation<_Safety>(index));
        }

        [[nodiscard]] ComponentOffset getComponentOffset(ComponentId id) const noexcept {
            ComponentOffset result;
            if (hasComponent(id)) {
                const auto index = getComponentIndex<FunctionSafety::kUnsafe>(id);
                result = operation_helper_.get[index].offset;
            }
            return result;
        }

        [[nodiscard]] const ComponentMask& componentMask() const noexcept {
            return mask_;
        }

        template<typename T, FunctionSafety _Safety = FunctionSafety::kDefault>
        T* getComponent(const ArchetypeInternalEntityLocation& location) const noexcept {
            return operation_helper_.getComponent<T, _Safety>(location);
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
            const auto chunk_capacity = operation_helper_.chunkCapacity();
            result.chunk = /*chunks_*/ data_storage_.chunks_[ChunkIndex::make(index.toInt() / chunk_capacity)];
            result.index = ChunkEntityIndex::make(index.toInt() % chunk_capacity);
            return result;
        }

        template<typename _F>
        void forEachEntity(_F&& callback) {
            if (data_storage_.isEmpty()) {
                return;
            }
            auto num_items = data_storage_.size();
            ArchetypeInternalEntityLocation location;
            const auto chunk_last_index = operation_helper_.index_of_last_entity_in_chunk;
            for (auto chunk : /*chunks_*/ data_storage_.chunks_) {
                location.chunk = chunk;
                for (location.index = ChunkEntityIndex::make(0); location.index <= chunk_last_index; ++location.index) {
                    if (num_items == 0) {
                        return;
                    }
                    callback(*operation_helper_.getEntity(location));
                    --num_items;
                }
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

        ComponentIndex componentIndex(ComponentId id) const noexcept {
            auto result = ComponentIndex::null();
            if (operation_helper_.component_id_to_component_index.has(id)) {
                result = operation_helper_.component_id_to_component_index[id];
            }
            return result;
        }
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponentFromArchetype(ArchetypeEntityIndex entity, ComponentIndex component) const noexcept {
            const auto location = entityIndexToInternalLocation(entity);
            return operation_helper_.getComponent<_Safety>(component, location);
        }
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponentFromArchetype(ArchetypeEntityIndex entity, ComponentId component) const noexcept {
            return getComponentFromArchetype<_Safety>(entity, componentIndex(component));
        }

        World& world_;
        ComponentMask mask_;
        ArchetypeOperationHelper operation_helper_;
        ComponentDataStorage data_storage_;

        ArchetypeIndex id_;
        std::string name_;
    };
}
