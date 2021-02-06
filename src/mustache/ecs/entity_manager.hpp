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

    struct ArchetypeChunkSize {
        uint32_t min = 0u;
        uint32_t max = 0u;
    };

    using ArchetypeChunkSizeFunction = std::function<ArchetypeChunkSize (const ComponentIdMask&)>;

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

        Archetype& getArchetype(const ComponentIdMask&, const SharedComponentsInfo& shared_component_mask);

        template<typename... ARGS>
        MUSTACHE_INLINE Archetype& getArchetype();

        [[nodiscard]] size_t MUSTACHE_INLINE getArchetypesCount() const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] MUSTACHE_INLINE  Archetype& getArchetype(ArchetypeIndex index) noexcept (!isSafe(_Safety));

        template<typename T>
        MUSTACHE_INLINE T* getComponent(Entity entity) const noexcept;

        template<typename T>
        MUSTACHE_INLINE const T* getSharedComponent(Entity entity) const noexcept;

        template<typename T, FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE bool removeComponent(Entity entity);

        bool removeComponent(Entity entity, ComponentId component);

        template<typename T, FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE bool removeSharedComponent(Entity entity);

        bool removeSharedComponent(Entity entity, SharedComponentId component);


        template<typename T, typename... _ARGS>
        MUSTACHE_INLINE decltype(auto) assign(Entity e, _ARGS&&... args) ;

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
            return archetype->getComponentVersion(location.index, component_id);
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

        ComponentIdMask getExtraComponents(const ComponentIdMask&) const noexcept;

        void addDependency(ComponentId master, const ComponentIdMask& dependent_mask) noexcept;

        template<typename _Master, typename... DEPENDENT>
        void addDependency() noexcept {
            const auto component_id = ComponentFactory::registerComponent<_Master>();
            const auto depend_on_mask = ComponentFactory::makeMask<DEPENDENT...>();
            addDependency(component_id, depend_on_mask);
        }

        void addChunkSizeFunction(const ArchetypeChunkSizeFunction& function);

        template <typename... ARGS>
        void addChunkSizeFunction(uint32_t min, uint32_t max) {
            const auto check_mask = ComponentFactory::makeMask<ARGS...>();
            addChunkSizeFunction([min, max, check_mask](const ComponentIdMask& arch_mask) noexcept {
                ArchetypeChunkSize result;
                if (arch_mask.isMatch(check_mask)) {
                    result.min = min;
                    result.max = max;
                }
                return result;
            });
        }

        void setDefaultArchetypeVersionChunkSize(uint32_t value) noexcept;

        template<typename T, typename... _ARGS>
        MUSTACHE_INLINE const T& assignShared(Entity e, _ARGS&&... args);

        template<typename T, typename... _ARGS>
        MUSTACHE_INLINE T& assignUnique(Entity e, _ARGS&&... args);

        MUSTACHE_INLINE SharedComponentPtr assignShared(Entity e, const SharedComponentPtr&, SharedComponentId id);
    private:

        SharedComponentPtr getCreatedSharedComponent(const SharedComponentPtr& ptr, SharedComponentId id);

        template<typename Component, typename TupleType, size_t... _I>
        void initComponent(Archetype& archetype, ArchetypeEntityIndex entity_index,
                           TupleType& tuple, std::index_sequence<_I...>&&) {
            if constexpr (std::tuple_size<TupleType>::value > 0 ||
                          !std::is_trivially_default_constructible<Component>::value) {
                static const auto component_id = ComponentFactory::registerComponent<Component>();
                const auto component_index = archetype.getComponentIndex<FunctionSafety::kUnsafe>(component_id);
                auto ptr = archetype.getComponent<FunctionSafety::kUnsafe>(component_index, entity_index);
                new(ptr) Component{std::get<_I>(tuple)...};
            }
        }

        template<class Arg>
        bool initComponent(Archetype& archetype, ArchetypeEntityIndex entity_index, Arg&& arg);

        template<typename TupleType, size_t... _I>
        void initComponents(Entity entity, TupleType& tuple, const SharedComponentsInfo&, std::index_sequence<_I...>&&);

        [[nodiscard]] WorldVersion worldVersion() const noexcept {
            return world_version_;
        }

        friend Archetype;
        void updateLocation(Entity e, ArchetypeIndex archetype, ArchetypeEntityIndex index) noexcept {
            if (e.id().isValid()) {
                if (!locations_.has(e.id())) {
                    locations_.resize(e.id().next().toInt());
                }
                auto& location = locations_[e.id()];
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

        ArrayWrapper<SharedComponentsData, SharedComponentId, false> shared_components_;
        using ArchetypeMap = std::map<SharedComponentsData, Archetype*>;
        std::map<ArchetypeComponents, ArchetypeMap> mask_to_arch_;
        std::map<ComponentId, ComponentIdMask > dependencies_;
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

        // archetype stores component version for every  default_archetype_component_version_chunk_size entities
        struct ArchetypeVersionChunkSize {
            uint32_t default_size = 1024u;
            uint32_t min_size = 0u;
            uint32_t max_size = 0u;
        };
        ArchetypeVersionChunkSize archetype_chunk_size_info_;
        std::vector<ArchetypeChunkSizeFunction> get_chunk_size_functions_;
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
        return getArchetype(ComponentFactory::makeMask<ARGS...>(), ComponentFactory::makeSharedInfo<ARGS...>());
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
        static_assert(!isComponentShared<Type>(), "Component is shared, use getSharedComponent() function");
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
        }

        auto ptr = arch->getComponent<FunctionSafety::kUnsafe>(index, location.index, worldVersion());
        auto typed_ptr = reinterpret_cast<T*>(ptr);
        return typed_ptr;
    }

    template<typename T>
    const T* EntityManager::getSharedComponent(Entity entity) const noexcept {
        using ComponentType = ComponentType<T>;
        using Type = typename ComponentType::type;
        static_assert(isComponentShared<Type>(), "Component is not shared, use getComponent() function");
        static const auto component_id = ComponentFactory::registerComponent<Type>();
        const auto& location = locations_[entity.id()];
        if (!location.archetype.isValid()) {
            return nullptr;
        }
        const auto& arch = archetypes_[location.archetype];

        auto ptr = arch->getSharedComponent(arch->sharedComponentIndex<T>());
        return static_cast<const T*>(ptr);
    }

    template<typename T, FunctionSafety _Safety>
    bool EntityManager::removeSharedComponent(Entity entity) {
        static const auto component_id = ComponentFactory::registerSharedComponent<T>();
        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return false;
            }
        }
        return removeSharedComponent(entity, component_id);
    }

    template<typename T, FunctionSafety _Safety>
    bool EntityManager::removeComponent(Entity entity) {
        static const auto component_id = ComponentFactory::registerComponent<T>();
        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return false;
            }
        }

        return removeComponent(entity, component_id);
    }

    template<typename T, typename... _ARGS>
    T& EntityManager::assignUnique(Entity e, _ARGS&&... args) {
        static const auto component_id = ComponentFactory::registerComponent<T>();
        const auto& location = locations_[e.id()];
        constexpr bool use_custom_constructor = sizeof...(_ARGS) > 0;

        auto& prev_arch = *archetypes_[location.archetype];
        ComponentIdMask mask = prev_arch.mask_;
        mask.add(component_id);
        auto& arch = getArchetype(mask, prev_arch.sharedComponentInfo());
        const auto prev_index = location.index;
        const auto skip_init_mask = use_custom_constructor ? ComponentIdMask{component_id} : ComponentIdMask{};
        arch.externalMove(e, prev_arch, prev_index, skip_init_mask);
        const auto component_index = arch.getComponentIndex<FunctionSafety::kUnsafe>(component_id);
        auto component_ptr = arch.getComponent<FunctionSafety::kUnsafe>(component_index, location.index);
        if constexpr (use_custom_constructor) {
            *new(component_ptr) T{std::forward<_ARGS>(args)...};
        }

        return *reinterpret_cast<T*>(component_ptr);
    }

    SharedComponentPtr EntityManager::assignShared(Entity e, const SharedComponentPtr& ptr, SharedComponentId id) {
        const auto& location = locations_[e.id()];

        auto component_data = getCreatedSharedComponent(ptr, id);
        auto& prev_arch = *archetypes_[location.archetype];

        SharedComponentsInfo shared_components_info = prev_arch.sharedComponentInfo();
        shared_components_info.add(id, component_data);

        auto& arch = getArchetype(prev_arch.componentMask(), shared_components_info);
        const auto prev_index = location.index;
        const ComponentIdMask skip_init_mask {};
        arch.externalMove(e, prev_arch, prev_index, skip_init_mask);
        return component_data;
    }

    template<typename T, typename... _ARGS>
    const T& EntityManager::assignShared(Entity e, _ARGS&&... args) {
        auto ptr = std::make_shared<T>(std::forward<_ARGS>(args)...);
        auto result = assignShared(e, ptr, ComponentFactory::registerSharedComponent<T>());
        return *static_cast<const T*>(result.get());
    }

    template<typename T, typename... _ARGS>
    decltype(auto) EntityManager::assign(Entity e, _ARGS&&... args) {
        if constexpr (isComponentShared<T>()) {
            return assignShared<T>(e, std::forward<_ARGS>(args)...);
        } else {
            return assignUnique<T>(e, std::forward<_ARGS>(args)...);
        }
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
        if constexpr (isComponentShared<T>()) {
            static const auto component_id = ComponentFactory::registerSharedComponent<T>();
            return archetype.hasComponent(component_id);
        } else {
            static const auto component_id = ComponentFactory::registerComponent<T>();
            return archetype.hasComponent(component_id);
        }
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
    void EntityManager::initComponents(Entity entity, TupleType& tuple,
                                       const SharedComponentsInfo& info, std::index_sequence<_I...>&&) {
        const auto mask = ComponentFactory::makeMask<typename std::tuple_element<_I, TupleType>::type::Component...>();
        Archetype& archetype = getArchetype(mask, info);
        const auto index = archetype.insert(entity, mask);
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
        const SharedComponentsInfo shared; // TODO: fill me
        initComponents<TupleType>(entity, args_pack.getArgs(), shared, std::make_index_sequence<num_components>());
        return entity;
    }
}
