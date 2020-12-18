#pragma once

#include <mustache/utils/default_settings.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/uncopiable.hpp>

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/base_job.hpp>
#include <mustache/ecs/archetype.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/entity_builder.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <memory>
#include <map>
#include <set>

namespace mustache {

    class World;
    class ComponentFactory;

    template<typename TupleType>
    class EntityBuilder;

    class EntityManager : public Uncopiable {
    public:
        explicit EntityManager(World& world);

        [[nodiscard]] Entity create();

        void update();

        // Checks if entity is valid for this World
        [[nodiscard]] MUSTACHE_INLINE bool isEntityValid(Entity entity) const noexcept;

        template<typename _F>
        MUSTACHE_INLINE void forEachArchetype(_F&& func) MUSTACHE_COPY_NOEXCEPT (func);

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
        [[nodiscard]] MUSTACHE_INLINE WorldVersion getWorldVersionOfLastComponentUpdate(Entity entity) const noexcept {
            const auto id = entity.id();
            if constexpr (isSafe(_Safety)) {
                if (entity.isNull() || !locations_.has(id)) {
                    return WorldVersion::null();
                }
            }
            const auto location = locations_[entity.id()];
            const auto& archetype = archetypes_[location.archetype];
            const auto component_id = ComponentFactory::registerComponent<T>();
            return archetype->getComponentVersion<_Safety>(location.index, component_id);
        }

        template<typename _F, typename... ARGS>
        MUSTACHE_INLINE void forEachWithArgsTypes(_F&& function, JobRunMode mode);

        template<typename _F, size_t... _I>
        MUSTACHE_INLINE void forEach(_F&& function, JobRunMode mode, std::index_sequence<_I...>&&) {
            using Info = utils::function_traits<_F>;
            forEachWithArgsTypes<_F, typename Info::template arg<_I>::type...>(std::forward<_F>(function), mode);
        }

        template<typename _F>
        MUSTACHE_INLINE void forEach(_F&& function, JobRunMode mode = JobRunMode::kDefault) {
            constexpr auto args_count = utils::function_traits<_F>::arity;
            forEach(std::forward<_F>(function), mode, std::make_index_sequence<args_count>());
        }

        EntityBuilder<void> begin() noexcept {
            return EntityBuilder<void>{this};
        }

        template<typename TupleType>
        Entity create(EntityBuilder<TupleType>& args_pack);

    private:
        template<typename Component, typename TupleType, size_t... _I>
        void initComponent(Archetype& archetype, ArchetypeEntityIndex entity_index,
                           TupleType& tuple, std::index_sequence<_I...>&&) {
            if constexpr (std::tuple_size<TupleType>::value > 0 ||
                          !std::is_trivially_default_constructible<Component>::value) {
                auto ptr = archetype.getComponent(ComponentFactory::registerComponent<Component>(), entity_index);
                new(ptr) Component{std::get<_I>(tuple)...};
            }
        }

        template<class Arg>
        bool initComponent(Archetype& archetype, ArchetypeEntityIndex entity_index, Arg&& arg);

        template<typename TupleType, size_t... _I>
        void initComponents(Entity entity, TupleType& tuple, std::index_sequence<_I...>&&);

        [[nodiscard]] WorldVersion worldVersion() const noexcept {
            return world_version_;
        }

        friend Archetype;
        void updateLocation(Entity e, ArchetypeIndex archetype, ArchetypeEntityIndex index) noexcept {
            if (e.id().isValid()) {
                if (!locations_.has(e.id())) {
                    locations_.resize(e.id().next().toInt());
                }
                auto &location = locations_[e.id()];
                location.archetype = archetype;
                location.index = index;
            }
        }

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
        ArrayWrapper<Entity, EntityId, true> entities_;
        ArrayWrapper<EntityLocationInWorld, EntityId, true> locations_;
        std::set<Entity, std::less<Entity>, Allocator<Entity> > marked_for_delete_;
        WorldId this_world_id_;
        WorldVersion world_version_;
        // TODO: replace shared pointed with some kind of unique_ptr but with deleter calling clearArchetype
        // NOTE: must be the last field(for correct default destructor).
        ArrayWrapper<std::shared_ptr<Archetype>, ArchetypeIndex, true> archetypes_;
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
    void EntityManager::forEachArchetype(_F&& func) MUSTACHE_COPY_NOEXCEPT(func) {
        for (auto& arh : archetypes_) {
            func(*arh);
        }
    }

    Entity EntityManager::create(Archetype& archetype) {
        Entity entity;
        if(!empty_slots_) {
            entity.reset(EntityId::make(entities_.size()), EntityVersion::make(0), this_world_id_);
            entities_.push_back(entity);
            updateLocation(entity, archetype.id(), archetype.insert(entity));
        } else {
            const auto id = next_slot_;
            const auto version = entities_[id].version();
            next_slot_ = entities_[id].id();
            entity.reset(id, version, this_world_id_);
            entities_[id] = entity;
            updateLocation(entity, archetype.id(), archetype.insert(entity));
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
        const auto& location = locations_[id];
        if (!location.archetype.isNull()) {
            if constexpr (isSafe(_Safety)) {
                if (!archetypes_.has(location.archetype)) {
                    throw std::runtime_error("Invalid archetype index");
                }
            }
            archetypes_[location.archetype]->remove(entity, location.index);
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
        using ComponentType = ComponentType<T>;
        using Type = typename ComponentType::type;
        static const auto component_id = ComponentFactory::registerComponent<Type>();
        const auto& location = locations_[entity.id()];
        if (!location.archetype.isValid()) {
            return nullptr;
        }
        const auto& arch = archetypes_[location.archetype];
        const auto index = arch->getComponentIndex(component_id);
        if (!index.isValid()) {
            return nullptr;
        }
        if constexpr (std::is_const<T>::value) {
            auto ptr = arch->getConstComponent<FunctionSafety::kUnsafe>(index, location.index);
            auto typed_ptr = reinterpret_cast<T*>(ptr);
            return typed_ptr;
        } else {
            auto ptr = arch->getComponent<FunctionSafety::kUnsafe>(index, location.index, worldVersion());
            auto typed_ptr = reinterpret_cast<T*>(ptr);
            return typed_ptr;
        }
    }

    template<typename T, FunctionSafety _Safety>
    void EntityManager::removeComponent(Entity entity) {
        static const auto component_id = ComponentFactory::registerComponent<T>();
        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return;
            }
        }
        const auto& location = locations_[entity.id()];
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
        auto& archetype = getArchetype(mask);
        const auto prev_index = location.index;

        archetype.externalMove(entity, prev_archetype, prev_index, false);
    }

    template<typename T, typename... _ARGS>
    T& EntityManager::assign(Entity e, _ARGS&&... args) {
        static const auto component_id = ComponentFactory::registerComponent<T>();
        const auto& location = locations_[e.id()];
        constexpr bool use_custom_constructor = sizeof...(_ARGS) > 0;

        auto& prev_arch = *archetypes_[location.archetype];
        ComponentIdMask mask = prev_arch.mask_;
        mask.add(component_id);
        auto& arch = getArchetype(mask);
        const auto prev_index = location.index;
        arch.externalMove(e, prev_arch, prev_index, !use_custom_constructor);
        auto component_ptr = arch.getComponent<FunctionSafety::kUnsafe>(component_id, location.index);
        if constexpr (use_custom_constructor) {
            *new(component_ptr) T{std::forward<_ARGS>(args)...};
        }

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

    template<typename TupleType>
    Entity EntityBuilder<TupleType>::end() {
        return entity_manager_->create(*this);
    }

    Entity EntityBuilder<void>::end() {
        return entity_manager_->create();
    }

    template<typename Component, typename... ARGS>
    auto EntityBuilder<void>::assign(ARGS&&... args) {
        using TypleType = decltype(std::forward_as_tuple(args...));
        using ComponentArgType = ComponentArg<Component, TypleType >;
        ComponentArgType arg {
                std::forward_as_tuple(args...)
        };
        return EntityBuilder <std::tuple<ComponentArgType> >(entity_manager_, std::move(arg));
    }

    template<typename TupleType, size_t... _I>
    void EntityManager::initComponents(Entity entity, TupleType& tuple, std::index_sequence<_I...>&&) {
        const auto mask = ComponentFactory::makeMask<typename std::tuple_element<_I, TupleType>::type::Component...>();
        Archetype& archetype = getArchetype(mask);
        const auto index = archetype.insert(entity, false);
        auto unused_init_list = {initComponent(archetype, index, std::get<_I>(tuple))...};
        (void ) unused_init_list;
    }

    template<class Arg>
    bool EntityManager::initComponent(Archetype& archetype, ArchetypeEntityIndex entity_index, Arg&& arg) {
        using ArgType = typename std::remove_reference<Arg>::type;
        using Component = typename ArgType::Component;
        constexpr size_t num_args = ArgType::num_args;
        initComponent<Component>(archetype, entity_index, arg.args, std::make_index_sequence<num_args>());
        return true;
    }

    template<typename TupleType>
    Entity EntityManager::create(EntityBuilder<TupleType>& args_pack) {
        Entity entity = create();
        constexpr size_t num_components = std::tuple_size<TupleType>();
        initComponents<TupleType>(entity, args_pack.getArgs(), std::make_index_sequence<num_components>());
        return entity;
    }
}
