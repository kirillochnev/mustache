#pragma once

#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/default_settings.hpp>

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/base_job.hpp>
#include <mustache/ecs/archetype.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/entity_builder.hpp>
#include <mustache/ecs/temporal_storage.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <map>
#include <set>
#include <memory>

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

    class MUSTACHE_EXPORT EntityManager : public Uncopiable {
    public:
        explicit EntityManager(World& world);

        /// iteration safe
        [[nodiscard]] MUSTACHE_INLINE Entity create() {
            return create<>();
        }

        void update();

        // Checks if entity is valid for this World
        /// iteration safe
        [[nodiscard]] MUSTACHE_INLINE bool isEntityValid(Entity entity) const noexcept;

        template<typename _F>
        MUSTACHE_INLINE void forEachArchetype(_F&& func) MUSTACHE_COPY_NOEXCEPT (func);

        /// iteration safe
        [[nodiscard]] MUSTACHE_INLINE Entity create(Archetype& archetype);

        /// iteration safe
        [[nodiscard]] MUSTACHE_INLINE Entity create(const ComponentIdMask& components, const SharedComponentsInfo& shared);

        /// iteration safe
        template<typename... Components>
        [[nodiscard]] MUSTACHE_INLINE Entity create();

        void clear();

        void clearArchetype(Archetype& archetype);

        /**
         * @brief Destroys an entity.
         *
         * This function is used to destroy the specified entity. If the EntityManager is currently locked,
         * the entity is added to the temporal storage for deletion. Otherwise, the entity is marked for deletion
         * and will be destroyed on the next update cycle.
         *
         * @param entity The entity to be destroyed.
         */
        MUSTACHE_INLINE void destroy(Entity entity);

        /**
         * @brief Check if an entity is marked for destruction.
         *
         * This function checks if the specified entity is marked for destruction.
         *
         * @param entity The entity to check.
         * @return True if the entity is marked for destruction, false otherwise.
         *
         * @note This function is iteration safe, which means that it can be safely called
         * during the iteration over entities.
         *
         * @see EntityManager
         */
        [[nodiscard]] MUSTACHE_INLINE bool isMarkedForDestroy(Entity entity) const noexcept;

        /// iteration safe
        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE void destroyNow(Entity entity);

        Archetype& getArchetype(const ComponentIdMask&, const SharedComponentsInfo& shared_component_mask);

        /// iteration safe
        Archetype* getArchetypeOf(Entity entity) const noexcept;

        template<typename... ARGS>
        MUSTACHE_INLINE Archetype& getArchetype();

        /// iteration safe
        [[nodiscard]] size_t MUSTACHE_INLINE getArchetypesCount() const noexcept;

        /// iteration safe
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] MUSTACHE_INLINE  Archetype& getArchetype(ArchetypeIndex index) noexcept (!isSafe(_Safety));

        /// iteration safe
        template<bool _Const = false, FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE auto getComponent(Entity entity, ComponentId) const noexcept;

        /// iteration safe
        template<typename T, FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE T* getComponent(Entity entity) const noexcept;

        /// iteration safe
        template<typename T>
        MUSTACHE_INLINE const T* getSharedComponent(Entity entity) const noexcept;

        /// iteration safe
        template<typename T, FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE void removeComponent(Entity entity);

        /// iteration safe
        void removeComponent(Entity entity, ComponentId component);

        /// NOT iteration safe
        template<typename T, FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE bool removeSharedComponent(Entity entity);

        /// NOT iteration safe
        bool removeSharedComponent(Entity entity, SharedComponentId component);


        /**
         * @brief Assigns a component to an entity.
         *
         * This function assigns a component to the specified entity. The component type
         * is deduced from the template parameter T. The function can also take additional
         * arguments, specified by the variadic parameter pack _ARGS.
         *
         * @tparam T The type of the component to assign.
         * @tparam _ARGS Additional arguments to be forwarded to the component constructor.
         * @param e The entity to assign the component to.
         * @param args Additional arguments to be forwarded to the component constructor.
         * @return The assigned component.
         *
         * @details This function assigns a component to the specified entity. If the component
         * type T is a shared component, it calls the assignShared function to assign the component.
         * Otherwise, it calls the assignUnique function to assign the component.
         *
         * @note This function is iteration safe, which means that it can be safely called
         * during the iteration over entities.
         *
         * @related EntityManager
         * @related assignShared
         * @related assignUnique
         */
        template<typename T, typename... _ARGS>
        MUSTACHE_INLINE decltype(auto) assign(Entity e, _ARGS&&... args);

        /// iteration safe
        MUSTACHE_INLINE void* assign(Entity e, ComponentId id) {
            return assign<false>(e, id);
        }

        MUSTACHE_INLINE void* assignWithoutInit(Entity e, ComponentId id) {
            return assign<true>(e, id);
        }

        /**
         * @brief Checks if the given entity has a component of type T.
         *
         * @tparam T The component type.
         * @tparam _Safety The function safety level.
         * @param entity The entity to check for the component.
         * @return bool Returns true if the entity has the component, false otherwise.
         *
         * This function checks if the specified entity has a component of the specified
         * type T. It returns true if the entity has the component, and false otherwise.
         * The function safety level _Safety determines the level of safety checks to perform.
         *
         * @note This function is iteration safe, which means that it can be safely called
         * during the iteration over entities.
         */
        template<typename T, FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] MUSTACHE_INLINE bool hasComponent(Entity entity) const noexcept;

        /**
         * @brief Checks if an entity has a specific component.
         *
         * This function checks if the specified entity has the specified component.
         *
         * @tparam _Safety The function safety level. By default, it uses the default safety level.
         * @tparam _ComponentId The ID of the component to check.
         * @param entity The entity to check.
         * @param id The ID of the component.
         * @return `true` if the entity has the specified component, `false` otherwise.
         *
         * @note This function is iteration safe, which means that it can be safely called
         * during the iteration over entities.
         *
         * @see EntityManager::hasComponent(Entity, _ComponentId)
         */
        template<FunctionSafety _Safety = FunctionSafety::kDefault, typename _ComponentId>
        [[nodiscard]] MUSTACHE_INLINE bool hasComponent(Entity entity, _ComponentId id) const noexcept;

        /**
         * @brief Returns the WorldVersion of the last update of a component for a given entity.
         *
         * This function retrieves the WorldVersion of the last update of a component for a given entity.
         * The FunctionSafety template parameter allows the user to specify the safety level of the function.
         *
         * @tparam T The component type.
         * @tparam _Safety The safety level of the function.
         * @param entity The entity for which to retrieve the WorldVersion.
         * @return The WorldVersion of the last update of the component for the entity.
         *
         * @note The function is iteration safe.
         * @note The function can throw exceptions if the safety level is not set to safe.
         */
        template<typename T, FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] MUSTACHE_INLINE WorldVersion getWorldVersionOfLastComponentUpdate(Entity entity) const noexcept;

        template<typename _F, typename... ARGS>
        MUSTACHE_INLINE void forEachWithArgsTypes(_F&& function, JobRunMode mode);
        /**
         * @brief Calls a given function on each element of a container.
         *
         * This function template is used to iterate over a container and apply a given
         * function to each element. The function to be applied must be callable with
         * the elements of the container as arguments. It is recommended that the
         * function supports forwarding arguments in order to handle any type of
         * elements.
         *
         * The function is called using the JobRunMode specified. The JobRunMode
         * determines how the iterations are executed:
         *   - kCurrentThread: The iterations are executed in the current thread.
         *   - kParallel: The iterations are executed in parallel using multiple threads.
         *   - kSingleThread: The iterations are executed sequentially in a single thread.
         *   - kDefault: The iterations are executed in the current thread (same as
         *               kCurrentThread).
         *
         * @tparam _F The type of the function to be applied.
         * @param function The function to be applied to each element.
         * @param mode The execution mode for the iterations.
         *
         * @see JobRunMode
         */
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
        /**
          * @fn EntityBuilder<void> begin(Entity entity = {})
          * @brief Begins the process of entity construction or modification.
          *
          * This function initiates the process of building or modifying an entity by creating an instance of the EntityBuilder class.
          * You can supply an already existing entity as an argument for modification. If no entity is provided, this function will create a new one automatically.
          *
          * You can add or remove components to/from the entity during the building process through the .assign and .remove methods respectively.
          * These operations change the entity's archetype, a major benefit of the EntityBuilder is that it allows you to place the entity into its final archetype directly, thus avoiding multiple costly archetype transitions.
          *
          * Example:
          * ```
          * world.entities().begin(existingEntity)
          *   .assign<C0>()
          *   .assign<C1>()
          *   .remove<C3>()
          * .end();
          * ```
          * In this example, the entity building process begins with an existing entity, then assigns two new components named C0 and C1 to the entity, removes the C3 component, and finally, ends the building process.
          *
          * @param entity The entity to build or modify (optional).
          * @return An instance of the EntityBuilder class.
          *
          * @see EntityBuilder
          */
        EntityBuilder<void> begin(Entity entity = {}) {
            return EntityBuilder<void>{this, entity};
        }

        template<typename TupleType>
        Entity apply(Entity entity, const ComponentIdMask& to_remove, EntityBuilder<TupleType>& args_pack);
        /**
         * @brief Retrieves the accumulated components that are dependent on a provided mask.
         *
         * This function returns a ComponentIdMask that represents the components having a
         * dependency on the given mask. It calculates this mask by integrating not only
         * the direct dependencies but also the indirect dependencies in a recursive manner,
         * until no new dependencies are discovered.
         *
         * @param mask The ComponentIdMask for which dependent components are to be identified.
         * @return A ComponentIdMask representing the accumulated dependent components.
         * This mask includes originally provided mask and all identified dependencies.
         */
        ComponentIdMask getExtraComponents(const ComponentIdMask&) const noexcept;

        /**
        * @brief Establishes a dependency between a master component and a set of dependent components.
        *
        * This function sets up a relationship where the master component has a dependency on a group of other components,
        * as represented through the dependent_mask. Once a dependency is established, the addition of the master component
        * automatically triggers the addition of all its direct and indirect dependent components.
        *
        * @param master The ID of the master component.
        * @param dependent_mask The mask representing the direct and indirect dependent components.
        *
        * @note This function does not throw exceptions.
        *
        * @see EntityManager::getExtraComponents
        *
        * @par Related Functions
        * - EntityManager::getExtraComponents
        */
        void addDependency(ComponentId master, const ComponentIdMask& dependent_mask) noexcept;

        /**
         * @brief Adds a dependency between a master component and one or more dependent components.
         *
         * @tparam _Master The master component type.
         * @tparam DEPENDENT The dependent component types.
         *
         * This function is used to establish a dependency relationship between a master component and one or more dependent components.
         * The master component should not be one of the dependent components.
         *
         * @note This function is noexcept.
         *
         * @see ComponentFactory
         *
         * @example
         *
         * // Example usage:
         *
         * // Define the master and dependent component types
         * struct MasterComponent {};
         * struct DependentComponent1 {};
         * struct DependentComponent2 {};
         *
         * // Add dependency between the master and dependent components
         * addDependency<MasterComponent, DependentComponent1, DependentComponent2>();
         *
         */
        template<typename _Master, typename... DEPENDENT>
        void addDependency() noexcept {
            static_assert(!IsOneOfTypes<_Master, DEPENDENT...>::value, "Self dependency is not allowed");
            const auto component_id = ComponentFactory::instance().registerComponent<_Master>();
            const auto depend_on_mask = ComponentFactory::instance().makeMask<DEPENDENT...>();
            addDependency(component_id, depend_on_mask);
        }
        /**
         * @brief Adds a chunk size function to the EntityManager.
         *
         * This function adds a chunk size function to the EntityManager. A chunk size function is a user-defined
         * function that specifies the size of each chunk in an archetype. It allows for customizing the chunk
         * size based on specific requirements or performance optimizations.
         *
         * @param function  The chunk size function to be added.
         */
        void addChunkSizeFunction(const ArchetypeChunkSizeFunction& function);

        /**
         * @brief Adds a chunk size function for the given archetype.
         *
         * This function generates a chunk size function based on the provided archetype component mask.
         * The chunk size function determines the minimum and maximum chunk sizes for the archetype based on the provided range.
         *
         * @tparam ARGS The component types in the archetype
         * @param min The minimum chunk size for the archetype
         * @param max The maximum chunk size for the archetype
         * @return void
         */
        template <typename... ARGS>
        void addChunkSizeFunction(uint32_t min, uint32_t max) {
            const auto check_mask = ComponentFactory::instance().makeMask<ARGS...>();
            addChunkSizeFunction([min, max, check_mask](const ComponentIdMask& arch_mask) noexcept {
                ArchetypeChunkSize result;
                if (arch_mask.isMatch(check_mask)) {
                    result.min = min;
                    result.max = max;
                }
                return result;
            });
        }

        /**
         * @brief Sets the default chunk size for archetype versioning.
         *
         * This function sets the default chunk size for archetype versioning.
         * The chunk size determines how many components are marked as dirty when a single component is modified.
         * When you modify one component, all the components in the same version chunk will be marked as dirty.
         *
         * @param value The new value for the default chunk size.
         *
         * @note The default chunk size will affect all the newly created entities.
         *
         * @see EntityManager::setDefaultArchetypeVersionChunkSize
         * @see archetype_chunk_size_info_
         */
        void setDefaultArchetypeVersionChunkSize(uint32_t value) noexcept;

        template<typename T, typename... _ARGS>
        MUSTACHE_INLINE const T& assignShared(Entity e, _ARGS&&... args);

        /// iteration safe
        template<typename T, typename... _ARGS>
        MUSTACHE_INLINE T& assignUnique(Entity e, _ARGS&&... args);

        MUSTACHE_INLINE SharedComponentPtr assignShared(Entity e, const SharedComponentPtr&, SharedComponentId id);

        /// iteration safe
        template<typename T>
        void markDirty(Entity entity) noexcept {
            markDirty<ComponentFactory::instance().registerComponent<T>()>(entity);
        }

        /// iteration safe
        void markDirty(Entity entity, ComponentId component_id) noexcept;

        [[nodiscard]] MUSTACHE_INLINE bool isLocked() const noexcept {
            return lock_counter_ > 0u;
        }

        /**
         * @brief Locks the entity manager, allowing changes to be applied to temporal storages.
         *
         * When the entity manager is locked, all changes made to the entities and components will be applied to temporal storages.
         * The entity manager has separate storages for different threads, so entities and components can be added or removed in parallel
         * mode and while iterating over them.
         *
         * @see EntityManager::unlock()
         */
        MUSTACHE_INLINE void lock() noexcept {
            ++lock_counter_;
            if (lock_counter_ == 1u) {
                onLock();
            }
        }

        /**
         * @brief Apply all changes
         *
         * This function applies all the pending changes in the entity manager.
         * It iterates through each temporal storage and applies the actions in the storage to update the entity manager's state.
         * The actions include creating and destroying entities, assigning and removing components, and moving entities between archetypes.
         */
        MUSTACHE_INLINE bool unlock() noexcept {
            if (lock_counter_ > 0u) {
                --lock_counter_;
            }
            if (lock_counter_ == 0u) {
                onUnlock();
            }
            return lock_counter_ < 1u;
        }

        Entity clone(const Entity& source, CloneEntityMap& entity_map) {
            if (!isEntityValid(source)) {
                return Entity{};
            }

            const auto location = locations_[source.id()];
            auto& arch = archetypes_[location.archetype];
            auto dest = createWithOutInit();
            entity_map.add(source, dest);
            arch->cloneEntity(source, dest, location.index, entity_map);
            return dest;
        }

        Entity clone(const Entity& source) {
            struct DefaultCloneEntityMap : CloneEntityMap{
                std::map<Entity, Entity> entities;
                void add(Entity src, Entity dst) override {
                    entities[src] = dst;
                }

                [[nodiscard]] Entity remap(Entity entity) const override {
                    const auto find_res = entities.find(entity);
                    if (find_res != entities.end()) {
                        return find_res->second;
                    }
                    return entity;
                }
            };
            DefaultCloneEntityMap entity_map {};
            return clone(source, entity_map);
        }

    private:
        /// iteration safe
        template<bool _SkipConstructor>
        MUSTACHE_INLINE void* assign(Entity e, ComponentId id);

        MUSTACHE_INLINE void releaseEntityIdUnsafe(Entity entity) noexcept {
            const auto id = entity.id();
            if (!entities_.has(id)) {
                entities_.resize(id.next().toInt());
            }
            entities_[id].reset(empty_slots_ ? next_slot_ : id.next(), entity.version().next());
            next_slot_ = id;
            ++empty_slots_;
        }

        void onLock();
        void onUnlock();

        void applyStorage(TemporalStorage& storage);
        void applyCommandPack(TemporalStorage& storage, size_t begin, size_t end);
        void applyCommandPackUnoptimized(TemporalStorage& storage, size_t begin, size_t end);

        Entity createLocked(const ComponentIdMask& components, const SharedComponentsInfo& shared) noexcept {
            // you need to store this entity in entities_ in onUnlock()
            const auto id = EntityId::make(next_entity_id_++);
            Entity entity;
            if (entities_.has(id)) {
                // version array can not be resized if entity manager is locked.
                entity.reset(id, entities_[id].version().next(), this_world_id_);
            }
            else {
                entity.reset(id, EntityVersion::make(0u), this_world_id_);
            }
            getTemporalStorage().create(entity, components, shared);
            return entity;
        }

        [[nodiscard]] ThreadId threadId() const noexcept;

        [[nodiscard]] TemporalStorage& getTemporalStorage() noexcept {
            static thread_local const auto thread_id = threadId();
            return temporal_storages_[thread_id];
        }

        [[nodiscard]] MUSTACHE_INLINE Entity createWithOutInit() noexcept;

        SharedComponentPtr getCreatedSharedComponent(const SharedComponentPtr& ptr, SharedComponentId id);

        template<typename Component, typename TupleType, size_t... _I>
        void initComponent(void* ptr, World& world, const Entity& e, TupleType& tuple, std::index_sequence<_I...>&&) {
            ComponentFactory::instance().initComponent<Component>(ptr, world, e,std::get<_I>(tuple)...);
        }

        template<typename Component, typename TupleType, size_t... _I>
        void initComponent([[maybe_unused]] Archetype& archetype, [[maybe_unused]] ArchetypeEntityIndex entity_index,
                           [[maybe_unused]] TupleType& tuple, std::index_sequence<_I...>&&) {

            constexpr bool call_custom_constructor = std::tuple_size<TupleType>::value > 0;
            constexpr bool call_constructor = call_custom_constructor || !std::is_trivially_default_constructible<Component>::value;
            constexpr bool call_default_constructor = !call_custom_constructor && !std::is_trivially_default_constructible<Component>::value;
            constexpr bool has_event = detail::hasAfterAssign<Component>(nullptr);

            if constexpr (call_constructor || has_event) {
                static const auto component_id = ComponentFactory::instance().registerComponent<Component>();
                auto view = archetype.getElementView(entity_index);
                const auto component_index = archetype.getComponentIndex<FunctionSafety::kUnsafe>(component_id);
                auto ptr = view.getData(component_index);
                if constexpr (call_custom_constructor) {
                    new(ptr) Component{ std::get<_I>(tuple)... };
                }
                if constexpr (call_default_constructor) {
                    ComponentInfo::componentConstructor<Component>(ptr, *view.getEntity(), world_);
                }
                if constexpr (has_event) {
                    ComponentInfo::afterComponentAssign<Component>(ptr, *view.getEntity(), world_);
                }
            }
        }

        // updateComponents and initComponents calls this
        template<typename Arg>
        bool initComponent(Archetype& archetype, ArchetypeEntityIndex entity_index, Arg&& arg);

        template<typename Arg>
        bool initComponent(void* ptr, World& world, const Entity& e, Arg&& arg);

        template<typename TupleType, size_t... _I>
        void initComponents(Entity entity, TupleType& tuple, const SharedComponentsInfo&,
                            const std::index_sequence<_I...>&);

        template<typename TupleType, size_t... _I>
        void updateComponents(const ComponentIdMask& to_remove, Entity entity, TupleType& tuple,
                              SharedComponentsInfo, const std::index_sequence<_I...>&);

        [[nodiscard]] WorldVersion worldVersion() const noexcept {
            return world_version_;
        }

        friend Archetype;
        void updateLocation(Entity e, ArchetypeIndex archetype, ArchetypeEntityIndex index) noexcept {
            if (e.id().isValid()) {
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
        uint32_t lock_counter_{0u};
        std::atomic<uint32_t > next_entity_id_; // for create entity with locked EntityManager
        ArrayWrapper<TemporalStorage, ThreadId, false> temporal_storages_;
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

        // archetype stores component version for every  default_archetype_component_version_chunk_size created_entities
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
    template<typename T, FunctionSafety _Safety>
    WorldVersion EntityManager::getWorldVersionOfLastComponentUpdate(Entity entity) const noexcept {
        const auto id = entity.id();
        if constexpr (isSafe(_Safety)) {
            if (entity.isNull() || !locations_.has(id)) {
                return WorldVersion::null();
            }
        }
        const auto location = locations_[entity.id()];
        const auto& archetype = archetypes_[location.archetype];
        const auto component_id = ComponentFactory::instance().registerComponent<T>();
        return archetype->getComponentVersion(location.index, component_id);
    }

    template<typename _F>
    void EntityManager::forEachArchetype(_F&& func) MUSTACHE_COPY_NOEXCEPT(func) {
        for (auto& arh : archetypes_) {
            func(*arh);
        }
    }

    Entity EntityManager::createWithOutInit() noexcept {
        if (!isLocked()) {
            Entity entity;
            if (!empty_slots_) {
                entity.reset(EntityId::make(entities_.size()), EntityVersion::make(0), this_world_id_);
                entities_.push_back(entity);
                locations_.emplace_back();
            } else {
                const auto id = next_slot_;
                const auto version = entities_[id].version();
                next_slot_ = entities_[id].id();
                entity.reset(id, version, this_world_id_);
                entities_[id] = entity;
                locations_[id] = EntityLocationInWorld{};
                --empty_slots_;
            }
            return entity;
        }
        return createLocked(ComponentIdMask::null(), SharedComponentsInfo::null());
    }

    Entity EntityManager::create(const ComponentIdMask& components, const SharedComponentsInfo& shared) {
        if (!isLocked()) {
            return create(getArchetype(components, shared));
        }
        return createLocked(components, shared);
    }

    Entity EntityManager::create(Archetype& archetype) {
        if (!isLocked()) {
            const Entity entity = createWithOutInit();
            archetype.insert(entity);
            return entity;
        }
        return createLocked(archetype.componentMask(), archetype.sharedComponentInfo());
    }


    template<typename... Components>
    Entity EntityManager::create() {
        const auto& factory = ComponentFactory::instance();
        return create(factory.makeMask<Components...>(), factory.makeSharedInfo<Components...>());
    }

    void EntityManager::destroy(Entity entity) {
        if (isLocked()) {
            getTemporalStorage().destroy(entity);
        } else {
            marked_for_delete_.insert(entity);
        }
    }

    bool EntityManager::isMarkedForDestroy(Entity entity) const noexcept {
        return marked_for_delete_.count(entity) > 0u;
    }

    template<FunctionSafety _Safety>
    void EntityManager::destroyNow(Entity entity) {
        if (isLocked()) {
            getTemporalStorage().destroyNow(entity);
            return;
        }

        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return;
            }
        }
        const auto& location = locations_[entity.id()];
        if (!location.archetype.isNull()) {
            if constexpr (isSafe(_Safety)) {
                if (!archetypes_.has(location.archetype)) {
                    throw std::runtime_error("Invalid archetype index");
                }
            }
            archetypes_[location.archetype]->remove(entity, location.index, ComponentIdMask::null());
        }
        releaseEntityIdUnsafe(entity);
    }

    template<typename... ARGS>
    Archetype& EntityManager::getArchetype() {
        const auto& factory = ComponentFactory::instance();
        return getArchetype(factory.makeMask<ARGS...>(), factory.makeSharedInfo<ARGS...>());
    }

    size_t EntityManager::getArchetypesCount() const noexcept {
        return archetypes_.size();
    }

    template<FunctionSafety _Safety>
    Archetype& EntityManager::getArchetype(ArchetypeIndex index) noexcept (!isSafe(_Safety)) {
        if constexpr (isSafe(_Safety)) {
            if (!archetypes_.has(index)) {
                throw std::runtime_error("Index is out of bound: " + std::to_string(index.toInt()));
            }
        }
        return *archetypes_[index];
    }

    template<bool _Const, FunctionSafety _Safety>
    auto EntityManager::getComponent(Entity entity, ComponentId component_id) const noexcept {
        using ResultType = typename std::conditional<_Const, const void*, void*>::type;
        if constexpr(isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return static_cast<ResultType>(nullptr);
            }
        }

        const auto& location = locations_[entity.id()];
        if constexpr(isSafe(_Safety)) {
            if (!location.archetype.isValid()) {
                return static_cast<ResultType>(nullptr);
            }
        }
        const auto& arch = archetypes_[location.archetype];
        const auto index = arch->getComponentIndex(component_id);
        if constexpr(isSafe(_Safety)) {
            if (!index.isValid()) {
                return static_cast<ResultType>(nullptr);
            }
        }

        if constexpr (_Const) {
            return arch->getConstComponent<FunctionSafety::kUnsafe>(index, location.index);
        } else {
            return arch->getComponent<FunctionSafety::kUnsafe>(index, location.index, worldVersion());
        }

    }

    template<typename T, FunctionSafety _Safety>
    T* EntityManager::getComponent(Entity entity) const noexcept {
        using ComponentType = ComponentType<T>;
        using Type = typename ComponentType::type;
        static_assert(!isComponentShared<Type>(), "Component is shared, use getSharedComponent() function");
        static const auto component_id = ComponentFactory::instance().registerComponent<Type>();
        auto ptr = getComponent<std::is_const<T>::value, _Safety>(entity, component_id);
        return static_cast<T*>(ptr);
    }

    template<typename T>
    const T* EntityManager::getSharedComponent(Entity entity) const noexcept {
        using ComponentType = ComponentType<T>;
        using Type = typename ComponentType::type;
        static_assert(isComponentShared<Type>(), "Component is not shared, use getComponent() function");
        static const auto component_id = ComponentFactory::instance().registerComponent<Type>();
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
        static const auto component_id = ComponentFactory::instance().registerSharedComponent<T>();
        if constexpr (isSafe(_Safety)) {
            if (!isEntityValid(entity)) {
                return false;
            }
        }
        return removeSharedComponent(entity, component_id);
    }

    template<typename T, FunctionSafety _Safety>
    void EntityManager::removeComponent(Entity entity) {
        static const auto component_id = ComponentFactory::instance().registerComponent<T>();
        if constexpr (isSafe(_Safety)) {
            if (!isLocked() && !isEntityValid(entity)) {
                return;
            }
        }

        removeComponent(entity, component_id);
    }

    template<bool _SkipConstructor>
    void* EntityManager::assign(Entity e, ComponentId component_id) {
        if (!isLocked()) {
            const auto& location = locations_[e.id()];
            auto& prev_arch = *archetypes_[location.archetype];
            ComponentIdMask mask = prev_arch.mask_;
            mask.add(component_id);
            auto& arch = getArchetype(mask, prev_arch.sharedComponentInfo());
            const auto prev_index = location.index;
            arch.externalMove(e, prev_arch, prev_index, _SkipConstructor ? mask : ComponentIdMask::null());
            const auto component_index = arch.getComponentIndex<FunctionSafety::kUnsafe>(component_id);
            return arch.getComponent<FunctionSafety::kUnsafe>(component_index, location.index);
        } else {
            return getTemporalStorage().assignComponent(world_, e, component_id, _SkipConstructor);
        }
    }

    template<typename T, typename... _ARGS>
    T& EntityManager::assignUnique(Entity e, _ARGS&&... args) {
        static const auto component_id = ComponentFactory::instance().registerComponent<T>();
        constexpr bool use_custom_constructor = sizeof...(_ARGS) > 0;
        auto component_ptr = assign<use_custom_constructor>(e, component_id);
        if constexpr(use_custom_constructor) {
            component_ptr = static_cast<void*>(new(component_ptr) T{std::forward<_ARGS>(args)...});
            ComponentInfo::afterComponentAssign<T>(component_ptr, e, world_);
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
        if (&arch != &prev_arch) {
            arch.externalMove(e, prev_arch, location.index, ComponentIdMask::null());
        }

        return component_data;
    }

    template<typename T, typename... _ARGS>
    const T& EntityManager::assignShared(Entity e, _ARGS&&... args) {
        auto ptr = std::make_shared<T>(std::forward<_ARGS>(args)...);
        auto result = assignShared(e, ptr, ComponentFactory::instance().registerSharedComponent<T>());
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

    template<FunctionSafety _Safety, typename _ComponentId>
    bool EntityManager::hasComponent(Entity entity, _ComponentId id) const noexcept {
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
        return archetype.hasComponent(id);
    }

    template<typename T, FunctionSafety _Safety>
    bool EntityManager::hasComponent(Entity entity) const noexcept {
        if constexpr (isComponentShared<T>()) {
            static const auto component_id = ComponentFactory::instance().registerSharedComponent<T>();
            return hasComponent<_Safety>(entity, component_id);
        } else {
            static const auto component_id = ComponentFactory::instance().registerComponent<T>();
            return hasComponent<_Safety>(entity, component_id);
        }
    }

    template<typename Component, typename... ARGS>
    auto EntityBuilder<void>::assign(ARGS&&... args) {
        using TypleType = decltype(std::forward_as_tuple(args...));
        using ComponentArgType = ComponentArg<Component, TypleType >;
        ComponentArgType arg {
                std::forward_as_tuple(args...)
        };
        return EntityBuilder <std::tuple<ComponentArgType> >(to_destroy_, entity_manager_, entity_, std::move(arg));
    }

    template<typename Arg>
    bool EntityManager::initComponent(Archetype& archetype, ArchetypeEntityIndex entity_index, Arg&& arg) {
        using ArgType = typename std::remove_reference<Arg>::type;
        using Component = typename ArgType::Component;
        constexpr size_t num_args = ArgType::num_args;
        initComponent<Component>(archetype, entity_index, arg.args, std::make_index_sequence<num_args>());
        return true;
    }
    template<typename Arg>
    bool EntityManager::initComponent(void* ptr, World& world, const Entity& e, Arg&& arg) {
        using ArgType = typename std::remove_reference<Arg>::type;
        using Component = typename ArgType::Component;
        constexpr size_t num_args = ArgType::num_args;
        initComponent<Component>(ptr, world, e, arg.args, std::make_index_sequence<num_args>());
        return true;
    }

    template<typename TupleType, size_t... _I>
    void EntityManager::initComponents(Entity entity, TupleType& tuple, const SharedComponentsInfo& info,
                                       const std::index_sequence<_I...>&) {
        if (isLocked()) {
            auto& storage = getTemporalStorage();
            static const std::array component_ids {
                ComponentFactory::instance().registerComponent<typename std::tuple_element<_I, TupleType>::type::Component>()...
            };
            const auto unused_init_list = {
                    initComponent(storage.assignComponent(world_, entity, component_ids[_I], true),
                                  world_, entity, std::get<_I>(tuple))...
            };
            (void) unused_init_list;
            return;
        }
        const auto mask = ComponentFactory::instance().makeMask<typename std::tuple_element<_I, TupleType>::type::Component...>();
        auto& archetype = getArchetype(mask, info);
        const auto index = archetype.insert(entity, mask);
        const auto unused_init_list = {initComponent(archetype, index, std::get<_I>(tuple))...};
        (void) unused_init_list;
    }

    template<typename TupleType, size_t... _I>
    void EntityManager::updateComponents(const ComponentIdMask& to_remove, Entity entity, TupleType& tuple,
                                         SharedComponentsInfo shared, const std::index_sequence<_I...>&) {
        if (isLocked()) {
            auto& storage = getTemporalStorage();
            if constexpr(sizeof...(_I) > 0u) {
                static const std::array component_ids {
                        ComponentFactory::instance().registerComponent<typename std::tuple_element<_I, TupleType>::type::Component>()...
                };
                const auto unused_init_list = {
                        initComponent(storage.assignComponent(world_, entity, component_ids[_I], true),
                                      world_, entity, std::get<_I>(tuple))...
                };
                (void) unused_init_list;
            }
            to_remove.forEachItem([&storage, entity](ComponentId id) {
                storage.removeComponent(entity, id);
            });
            return;
        }
        ComponentIdMask skip_mask;
        if constexpr(sizeof...(_I) > 0) {
            skip_mask = ComponentFactory::instance().makeMask<typename std::tuple_element<_I, TupleType>::type::Component...>();
        }
        const auto& prev_location = locations_[entity.id()];
        auto prev_arch = archetypes_[prev_location.archetype].get();
        const auto mask = skip_mask.merge(prev_arch->mask_).intersection(to_remove.inverse());
        shared = shared.merge(prev_arch->sharedComponentInfo());
        auto& archetype = getArchetype(mask, shared);
        archetype.externalMove(entity, *prev_arch, prev_location.index, ComponentIdMask::null());
        const auto index = locations_[entity.id()].index;
        if constexpr(sizeof...(_I) > 0) {
            auto unused_init_list = {initComponent(archetype, index, std::get<_I>(tuple))...};
            (void) unused_init_list;
        }
    }

    template<typename TupleType>
    Entity EntityManager::apply(Entity entity, const ComponentIdMask& to_remove, EntityBuilder<TupleType>& args_pack) {
        const SharedComponentsInfo shared; // TODO: fill me
        if constexpr (std::is_same<TupleType, void>::value) {
            if (entity.isNull()) {
                return create();
            }
            updateComponents(to_remove, entity, entity, shared, std::make_index_sequence<0>());
        } else {
            constexpr size_t num_components = std::tuple_size<TupleType>();
            constexpr auto index_sequence = std::make_index_sequence<num_components>();
            if (entity.isNull()) {
                entity = createWithOutInit();
                initComponents<TupleType>(entity, args_pack.getArgs(), shared, index_sequence);
            } else {
                updateComponents(to_remove, entity, args_pack.getArgs(), shared, index_sequence);
            }
        }
        return entity;
    }

    template<typename TupleType>
    Entity EntityBuilder<TupleType>::end() {
        return entity_manager_->apply(entity_, to_destroy_, *this);
    }

    Entity EntityBuilder<void>::end() {
        return entity_manager_->apply(entity_, to_destroy_, *this);
    }

}
