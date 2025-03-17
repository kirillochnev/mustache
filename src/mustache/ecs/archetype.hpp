#pragma once

#include <mustache/utils/type_info.hpp>
#include <mustache/utils/uncopiable.hpp>

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/entity_group.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/component_version_storage.hpp>
#include <mustache/ecs/archetype_operation_helper.hpp>
#include <mustache/ecs/base_component_data_storage.hpp>
#include <mustache/ecs/default_component_data_storage.hpp>

#include <stdexcept>
#include <cstdint>
#include <string>

namespace mustache {

    class World;
    class Archetype;
    class EntityManager;
    struct CloneEntityMap;

    // NOTE: element view does not update component versions
    struct MUSTACHE_EXPORT ElementView : public DataStorageIterator {
        using DataStorageIterator::DataStorageIterator;

        ElementView(const DataStorageIterator& view, const Archetype& archetype):
                DataStorageIterator{view},
                archetype_{&archetype} {

        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE Entity* getEntity() const;


        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE const SharedComponentTag* getSharedComponent(SharedComponentIndex index) const noexcept;

        const Archetype* archetype_ = nullptr;
    };

    using ArchetypeFilterParam = MaskAndVersion;

    struct ArchetypeComponents {
        ComponentIdMask unique;
        SharedComponentIdMask shared;
    };
    /**
     * Stores Entities with same component set
     * NOTE: It is has no information about entity manager, so Archetype's methods don't effects entity location.
     */
    class MUSTACHE_EXPORT Archetype : public Uncopiable {
    public:
        Archetype(World& world, ArchetypeIndex id, const ComponentIdMask& mask,
                  const SharedComponentsInfo& shared_components_info, uint32_t chunk_size);
        ~Archetype();

        [[nodiscard]] EntityGroup createGroup(size_t count);

        [[nodiscard]] uint32_t size() const noexcept {
            return static_cast<uint32_t>(entities_.size());
        }

        [[nodiscard]] uint32_t capacity() const noexcept;

        [[nodiscard]] ArchetypeIndex id() const noexcept;

        [[nodiscard]] uint32_t chunkCount() const noexcept;

        [[nodiscard]] ChunkCapacity chunkCapacity() const noexcept;

        [[nodiscard]] bool isMatch(const ComponentIdMask& mask) const noexcept;

        [[nodiscard]] bool isMatch(const SharedComponentIdMask& mask) const noexcept;

        [[nodiscard]] bool hasComponent(ComponentId component_id) const noexcept;

        [[nodiscard]] bool hasComponent(SharedComponentId component_id) const noexcept;

        WorldVersion worldVersion() const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] ComponentIndex getComponentIndex(ComponentId id) const noexcept {
            return operation_helper_.componentIndex<_Safety>(id);
        }

        [[nodiscard]] const ComponentIdMask& componentMask() const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE Entity* entityAt(ArchetypeEntityIndex index);

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        const void* getConstComponent(ComponentIndex component_index, ArchetypeEntityIndex index) const noexcept {
            return data_storage_->getData<_Safety>(component_index, ComponentStorageIndex::fromArchetypeIndex(index));
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponent(ComponentIndex component_index, ArchetypeEntityIndex index, WorldVersion version) noexcept;

        [[nodiscard]] ElementView getElementView(ArchetypeEntityIndex index) const noexcept;

        [[nodiscard]] WorldVersion getComponentVersion(ArchetypeEntityIndex index, ComponentId id) const noexcept;

        [[nodiscard]] ChunkIndex lastChunkIndex() const noexcept;

        [[nodiscard]] ComponentIndexMask makeComponentMask(const ComponentIdMask& mask) const noexcept;

        [[nodiscard]] auto& versionStorage() noexcept {
            return version_storage_;
        }

        [[nodiscard]] const auto& versionStorage() const noexcept {
            return version_storage_;
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] MUSTACHE_INLINE const SharedComponentTag* getSharedComponent(SharedComponentIndex index) const noexcept;

        template<typename T>
        [[nodiscard]] MUSTACHE_INLINE const T* getSharedComponent() const noexcept;

        [[nodiscard]] MUSTACHE_INLINE SharedComponentIndex sharedComponentIndex(SharedComponentId id) const noexcept;

        template<typename T>
        [[nodiscard]] MUSTACHE_INLINE SharedComponentIndex sharedComponentIndex() const noexcept;

        template<typename... ARGS>
        MUSTACHE_INLINE bool getSharedComponents(std::tuple<ARGS...>& out) const;

        [[nodiscard]] const SharedComponentsInfo& sharedComponentInfo() const noexcept {
            return shared_components_info_;
        }

        [[nodiscard]] const ArrayWrapper<Entity, ArchetypeEntityIndex, true>& entities() const noexcept;

    private:

        MUSTACHE_INLINE void markComponentDirty(ComponentIndex component, ArchetypeEntityIndex index,
                                                WorldVersion version) noexcept {
            versionStorage().setVersion(version, index, component);
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponent(ComponentIndex component, ArchetypeEntityIndex index) const noexcept {
            return data_storage_->getData<_Safety>(component, ComponentStorageIndex::fromArchetypeIndex(index));
        }

        friend ElementView;
        friend EntityManager;

        [[nodiscard]] ComponentStorageIndex pushBack(Entity entity);

        void popBack();

        [[nodiscard]] bool isEmpty() const noexcept;

        void clear();

        /// Entity must belong to default(empty) archetype
        ArchetypeEntityIndex insert(Entity entity, const ComponentIdMask& skip_constructor = ComponentIdMask::null());

        // Move from prev to this archetype
        void externalMove(Entity entity, Archetype& prev, ArchetypeEntityIndex prev_index,
                          const ComponentIdMask& skip_constructor);
        void internalMove(ArchetypeEntityIndex from, ArchetypeEntityIndex to);
        /**
         * removes entity from archetype, calls destructor for each trivially destructible component
         * moves last entity at index.
         * returns new entity at index.
         */
        void remove(Entity entity, ArchetypeEntityIndex index, const ComponentIdMask& skip_on_remove_call);
        void callDestructor(const ElementView& view);
        void callOnRemove(ArchetypeEntityIndex index, const ComponentIdMask& components_to_be_removed);

        void cloneEntity(Entity source, Entity dest, ArchetypeEntityIndex index, CloneEntityMap& map);

        World& world_;
        const ComponentIdMask mask_;
        const SharedComponentsInfo shared_components_info_;
        VersionStorage<true> version_storage_;
        ArchetypeOperationHelper operation_helper_;
        std::unique_ptr<DefaultComponentDataStorage> data_storage_;
        ArrayWrapper<Entity, ArchetypeEntityIndex, true> entities_;
        const ArchetypeIndex id_;
    };

    template<FunctionSafety _Safety>
    void* Archetype::getComponent(ComponentIndex component_index, ArchetypeEntityIndex index,
                                  WorldVersion version) noexcept {
        auto res = data_storage_->getData<_Safety>(component_index, ComponentStorageIndex::fromArchetypeIndex(index));
        if (res != nullptr) {
            markComponentDirty(component_index, index, version);
        }

        return res;
    }

    template<FunctionSafety _Safety>
    Entity* Archetype::entityAt(ArchetypeEntityIndex index) {
        if constexpr (isSafe(_Safety)) {
            if (!index.isValid() || !entities_.has(index)) {
                return nullptr;
            }
        }
        return entities_.data() + index.toInt();
    }

    template<FunctionSafety _Safety>
    const SharedComponentTag* Archetype::getSharedComponent(SharedComponentIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (!index.isValid()) {
                return nullptr;
            }
            return shared_components_info_.get(index).get();
        }
        return nullptr;
    }
    template<typename T>
    const T* Archetype::getSharedComponent() const noexcept {
        using Type = typename ComponentType<T>::type;
        const auto index = sharedComponentIndex<Type>();
        const auto ptr = getSharedComponent(index);
        return static_cast<const T*>(ptr);
    }

    SharedComponentIndex Archetype::sharedComponentIndex(SharedComponentId id) const noexcept {
        SharedComponentIndex result = shared_components_info_.indexOf(id);
        return result;
    }

    template<typename T>
    SharedComponentIndex Archetype::sharedComponentIndex() const noexcept {
        using Component = typename ComponentType<T>::type;
        return sharedComponentIndex(ComponentFactory::instance().registerSharedComponent<Component>());
    }

    template<typename... ARGS>
    bool Archetype::getSharedComponents(std::tuple<ARGS...>& out) const {
        out = std::make_tuple(static_cast<ARGS>(getSharedComponent(sharedComponentIndex<ARGS>()))...);
        return true;
    }

    template<FunctionSafety _Safety>
    MUSTACHE_INLINE const SharedComponentTag* ElementView::getSharedComponent(SharedComponentIndex index) const noexcept {
        return archetype_->template getSharedComponent<_Safety>(index);
    }

    template<FunctionSafety _Safety>
    Entity* ElementView::getEntity() const {
        return const_cast<Archetype*>(archetype_)->entityAt(global_index_.toArchetypeIndex());
    }
}
