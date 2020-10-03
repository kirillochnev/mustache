#pragma once

#include <mustache/utils/default_settings.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/uncopiable.hpp>

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/archetype.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <memory>
#include <map>
#include <set>

namespace mustache {

    class World;
    class ComponentFactory;

    class EntityManager : public Uncopiable {
    public:
        explicit EntityManager(World& world);

        [[nodiscard]] Entity create();

        void update();

        // Checks if entity is valid for this World
        [[nodiscard]] MUSTACHE_INLINE bool isEntityValid(Entity entity) const noexcept;

        template<typename _F>
        MUSTACHE_INLINE void forEachArchetype(_F&& func) noexcept (noexcept(func));

        [[nodiscard]] MUSTACHE_INLINE Entity create(Archetype& archetype);

        template<typename... Components>
        [[nodiscard]] MUSTACHE_INLINE Entity create();

        void clear();

        void clearArchetype(Archetype& archetype);

        MUSTACHE_INLINE void destroy(Entity entity);

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE void destroyNow(Entity entity);

        Archetype& getArchetype(const ComponentIdMask&);

        template<typename... ARGS>
        MUSTACHE_INLINE Archetype& getArchetype();

        [[nodiscard]] size_t MUSTACHE_INLINE getArchetypesCount() const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] MUSTACHE_INLINE  Archetype& getArchetype(ArchetypeIndex index) noexcept (!isSafe(_Safety));

        template<typename T>
        MUSTACHE_INLINE T* getComponent(Entity entity) const noexcept;

        template<typename T, FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE void removeComponent(Entity entity);

        template<typename T, typename... _ARGS>
        MUSTACHE_INLINE T& assign(Entity e, _ARGS&&... args) ;

        template<typename T, FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] MUSTACHE_INLINE bool hasComponent(Entity entity) const noexcept;

        template<typename T, FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] MUSTACHE_INLINE WorldVersion getWorldVersionOfLastComponentUpdate(Entity entity) const noexcept ;
    private:
        struct EntityLocationInWorld {
            constexpr static ArchetypeIndex kDefaultArchetype = ArchetypeIndex::null();
            EntityLocationInWorld() = default;
            EntityLocationInWorld(ArchetypeEntityIndex i, ArchetypeIndex arch ) noexcept :
                    index{i},
                    archetype{arch} {

            }
            ArchetypeEntityIndex index = ArchetypeEntityIndex::make(0u);
            ArchetypeIndex archetype = kDefaultArchetype;
        };

        World& world_;
        std::map<ComponentIdMask , Archetype* > mask_to_arch_;
        EntityId next_slot_ = EntityId::make(0);

        uint32_t empty_slots_{0};
        ArrayWrapper<std::vector<Entity>, EntityId> entities_;
        ArrayWrapper<std::vector<EntityLocationInWorld>, EntityId> locations_;
        std::set<Entity> marked_for_delete_;
        WorldId this_world_id_;
        // TODO: replace shared pointed with some kind of unique_ptr but with deleter calling clearArchetype
        // NOTE: must be the last field(for correct default destructor).
        ArrayWrapper<std::vector<std::shared_ptr<Archetype> >, ArchetypeIndex> archetypes_;
    };


    bool EntityManager::isEntityValid(Entity entity) const noexcept {
        const auto id = entity.id();  // it is ok to get id for no-valid entity, null id will be returned
        if (entity.isNull() || entity.worldId() != this_world_id_ ||
            !entities_.has(id) || entities_[id].version() != entity.version()) {
            return false;
        }
        return true;
    }

    template<typename _F>
    void EntityManager::forEachArchetype(_F&& func) noexcept (noexcept(func)) {
        for (auto& arh : archetypes_) {
            func(*arh);
        }
    }

    Entity EntityManager::create(Archetype& archetype) {
        Entity entity;
        if(!empty_slots_) {
            entity.reset(EntityId::make(entities_.size()), EntityVersion::make(0), this_world_id_);
            entities_.push_back(entity);
            locations_.emplace_back(archetype.insert(entity), archetype.id());
        } else {
            const auto id = next_slot_;
            const auto version = entities_[id].version();
            next_slot_ = entities_[id].id();
            entity.reset(id, version, this_world_id_);
            entities_[id] = entity;
            locations_[id].archetype = archetype.id();
            locations_[id].index = archetype.insert(entity);
            --empty_slots_;
        }
        return entity;
    }

    template<typename... Components>
    Entity EntityManager::create() {
        return create(getArchetype<Components...>());
    }

    void EntityManager::destroy(Entity entity) {
        marked_for_delete_.insert(entity);
    }

    template<FunctionSafety _Safety>
    void EntityManager::destroyNow(Entity entity) {
        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return;
            }
        }
        const auto id = entity.id();
        auto& location = locations_[id];
        if (!location.archetype.isNull()) {
            if constexpr (isSafe(_Safety)) {
                if (!archetypes_.has(location.archetype)) {
                    throw std::runtime_error("Invalid archetype index");
                }
            }
            auto& archetype = archetypes_[location.archetype];
            auto moved_entity =  archetype->remove(location.index);
            if (!moved_entity.isNull()) {
                locations_[moved_entity.id()] = location;
            }
            location.archetype = ArchetypeIndex::null();
        }
        entities_[id].reset(empty_slots_ ? next_slot_ : id.next(), entity.version().next());
        next_slot_ = id;
        ++empty_slots_;
    }
    template<typename... ARGS>
    Archetype& EntityManager::getArchetype() {
        return getArchetype(ComponentFactory::makeMask<ARGS...>());
    }

    size_t EntityManager::getArchetypesCount() const noexcept {
        return archetypes_.size();
    }

    template<FunctionSafety _Safety>
    Archetype& EntityManager::getArchetype(ArchetypeIndex index) noexcept (!isSafe(_Safety)) {
        if constexpr (isSafe(_Safety)) {
            if (!archetypes_.has(index)) {
                throw std::runtime_error("Index is out of bound");
            }
        }
        return *archetypes_[index];
    }

    template<typename T>
    T* EntityManager::getComponent(Entity entity) const noexcept {
        static const auto component_id = ComponentFactory::registerComponent<T>();
        const auto& location = locations_[entity.id()];
        if (!location.archetype.isValid()) {
            return nullptr;
        }
        const auto& arch = archetypes_[location.archetype];
        const auto index = arch->getComponentIndex(component_id);
        if (!index.isValid()) {
            return nullptr;
        }
        auto ptr = arch->getComponent<FunctionSafety::kUnsafe>(index, location.index);
        return reinterpret_cast<T*>(ptr);
    }

    template<typename T, FunctionSafety _Safety>
    void EntityManager::removeComponent(Entity entity) {
        static const auto component_id = ComponentFactory::registerComponent<T>();
        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return;
            }
        }
        auto& location = locations_[entity.id()];
        if constexpr (isSafe(_Safety)) {
            if (location.archetype.isNull()) {
                return;
            }
        }
        auto& prev_archetype = *archetypes_[location.archetype];
        auto mask = prev_archetype.mask_;
        if constexpr (isSafe(_Safety)) {
            if (!mask.has(component_id)) {
                return;
            }
        }
        mask.set(component_id, false);
        if (mask.isEmpty()) {
            prev_archetype.remove(location.index);
            location.archetype = ArchetypeIndex::null();
            location.index = ArchetypeEntityIndex::null();
            return;
        }
        auto& archetype = getArchetype(mask);
        const auto prev_index = location.index;

        location.index = archetype.insert(entity, prev_archetype, prev_index, false);
        auto moved_entity = prev_archetype.entityAt(prev_index);
        if (!moved_entity.isNull()) {
            locations_[moved_entity.id()].index = prev_index;
            locations_[moved_entity.id()].archetype = location.archetype;
        }
        location.archetype = archetype.id();
    }

    template<typename T, typename... _ARGS>
    T& EntityManager::assign(Entity e, _ARGS&&... args) {
        static const auto component_id = ComponentFactory::registerComponent<T>();
        auto& location = locations_[e.id()];
        constexpr bool use_custom_constructor = sizeof...(_ARGS) > 0;
        if (location.archetype.isNull()) {
            static const ComponentIdMask mask{component_id};
            auto& arch = getArchetype(mask);
            location.archetype = arch.id();
            location.index = arch.insert(e, !use_custom_constructor);
            auto component_ptr = arch.getComponent<FunctionSafety::kUnsafe>(component_id, location.index);
            if constexpr (use_custom_constructor) {
                *new(component_ptr) T{std::forward<_ARGS>(args)...};
            }
            return *reinterpret_cast<T*>(component_ptr);
        }
        auto& prev_arch = *archetypes_[location.archetype];
        ComponentIdMask mask = prev_arch.mask_;
        mask.add(component_id);
        auto& arch = getArchetype(mask);
        const auto prev_index = location.index;
        location.index = arch.insert(e, prev_arch, prev_index, !use_custom_constructor);
        auto component_ptr = arch.getComponent<FunctionSafety::kUnsafe>(component_id, location.index);
        if constexpr (use_custom_constructor) {
            *new(component_ptr) T{std::forward<_ARGS>(args)...};
        }
        auto moved_entity = prev_arch.entityAt<FunctionSafety::kSafe>(prev_index);
        if (!moved_entity.isNull()) {
            locations_[moved_entity.id()].index = prev_index;
            locations_[moved_entity.id()].archetype = location.archetype;
        }
        location.archetype = arch.id();
        return *reinterpret_cast<T*>(component_ptr);
    }

    template<typename T, FunctionSafety _Safety>
    bool EntityManager::hasComponent(Entity entity) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return false;
            }
        }
        const auto& location = locations_[entity.id()];
        if (location.archetype.isNull()) {
            return false;
        }
        const auto& archetype = *archetypes_[location.archetype];
        return archetype.hasComponent<T>();
    }

    template<typename T, FunctionSafety _Safety>
    WorldVersion EntityManager::getWorldVersionOfLastComponentUpdate(Entity entity) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return WorldVersion::null();
            }
        }
        const auto component_id = ComponentFactory::registerComponent<T>();
        const auto& location = locations_[entity.id()];
        const auto& archetype = *archetypes_[location.archetype];
        return archetype.getWorldVersionComponentUpdate<_Safety>(component_id, location.index);
    }
}