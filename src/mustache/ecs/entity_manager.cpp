#include "entity_manager.hpp"

#include <mustache/utils/profiler.hpp>

#include <mustache/ecs/world.hpp>

using namespace mustache;

namespace mustache {
    bool operator<(const ArchetypeComponents& lhs, const ArchetypeComponents& rhs) noexcept {
        return lhs.unique < rhs.unique;
    }
}

EntityManager::EntityManager(World& world):
        world_{world},
        entities_{world.memoryManager()},
        locations_{world.memoryManager()},
        marked_for_delete_{world.memoryManager()},
        this_world_id_{world.id()},
        world_version_{world.version()},
        archetypes_{world.memoryManager()} {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );
}

Archetype& EntityManager::getArchetype(const ComponentIdMask& mask, const SharedComponentsInfo& shared) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__ );
    const ComponentIdMask arch_mask = mask.merge(getExtraComponents(mask));
    ArchetypeComponents archetype_components;
    archetype_components.unique = arch_mask;
    archetype_components.shared = shared.mask();
    auto& map = mask_to_arch_[archetype_components];
    auto& result = map[shared.data()];
    if(result) {
        return *result;
    }
    {
        MUSTACHE_PROFILER_BLOCK_LVL_0("Create archetype");
        auto deleter = [this](Archetype* archetype) {
            clearArchetype(*archetype);
            delete archetype;
        };

        auto min = archetype_chunk_size_info_.min_size;
        auto max = archetype_chunk_size_info_.max_size;

        for (const auto& func: get_chunk_size_functions_) {
            const auto size = func(arch_mask);
            if (min == 0 || size.min > min) {
                min = size.min;
            }

            if (max == 0 || (size.max > 0 && size.max < max)) {
                max = size.max;
            }
        }
        auto chunk_size = archetype_chunk_size_info_.default_size;

        if (max < min) {
            throw std::runtime_error("Can not create archetype: "
                                     + std::to_string(max) + " < " + std::to_string(min));
        }

        if (chunk_size < min) {
            chunk_size = min;
        }

        if (max > 0 && chunk_size > max) {
            chunk_size = max;
        }

        result = new Archetype(world_, archetypes_.back_index().next(),
                               arch_mask, shared, chunk_size);
        archetypes_.emplace_back(result, deleter);
    }
    return *result;
}

void EntityManager::clear() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    entities_.clear();
    locations_.clear();
    next_slot_ = EntityId::make(0);
    empty_slots_ = 0u;
    for(auto& arh : archetypes_) {
        arh->clear();
    }
}

void EntityManager::update() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    if (isLocked()) {
        throw std::runtime_error("Can not update locked EntityManager");
    }
    world_version_ = world_.version();
    for (auto entity : marked_for_delete_) {
        destroyNow(entity);
    }
    marked_for_delete_.clear();
}

void EntityManager::clearArchetype(Archetype& archetype) {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    for (const auto entity : archetype.entities()) {
        const auto id = entity.id();
        auto& location = locations_[id];
        location.archetype = ArchetypeIndex::null();
        entities_[id].reset(empty_slots_ ? next_slot_ : id.next(), entity.version().next());
        next_slot_ = id;
        ++empty_slots_;
    }
    archetype.clear();
}

void EntityManager::addDependency(ComponentId component, const ComponentIdMask& extra) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );

    auto& dependency = dependencies_[component];
    dependency = dependency.merge(extra.merge(getExtraComponents(extra)));
}

void EntityManager::addChunkSizeFunction(const ArchetypeChunkSizeFunction& function) {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    if (function) {
        /// TODO: update archetypes
        get_chunk_size_functions_.push_back(function);
    }
}

void EntityManager::setDefaultArchetypeVersionChunkSize(uint32_t value) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );

    /// TODO: update archetypes
    archetype_chunk_size_info_.default_size = value;
}

ComponentIdMask EntityManager::getExtraComponents(const ComponentIdMask& mask) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );

    ComponentIdMask result;
    auto cur_mask = mask;
    if (!dependencies_.empty()) {
        ComponentIdMask prev_mask;
        do {
            prev_mask = result;
            cur_mask.forEachItem([&result, this](ComponentId id) {
                const auto find_res = dependencies_.find(id);
                if (find_res != dependencies_.end()) {
                    result = result.merge(find_res->second);
                }
            });
            cur_mask = result;
        } while (prev_mask != result);
    }
    return result;
}

SharedComponentPtr EntityManager::getCreatedSharedComponent(const SharedComponentPtr& ptr, SharedComponentId id) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );

    if (!shared_components_.has(id)) {
        shared_components_.resize(id.next().toInt());
    }
    auto& arr = shared_components_[id];
    for (const auto& v : arr) {
        if (v.get() == ptr.get() || ComponentFactory::isEq(v.get(), ptr.get(), id)) {
            return v;
        }
    }
    arr.push_back(ptr);
    return ptr;
}

void EntityManager::removeComponent(Entity entity, ComponentId component) {
    MUSTACHE_PROFILER_BLOCK_LVL_1(__FUNCTION__ );

    if (isLocked()) {
        getTemporalStorage().removeComponent(entity, component);
        return;
    }
    const auto& location = locations_[entity.id()];
    if (location.archetype.isNull()) {
        return;
    }

    auto& prev_archetype = *archetypes_[location.archetype];
    if (!prev_archetype.hasComponent(component)) {
        return;
    }

    auto mask = prev_archetype.mask_;
    mask.set(component, false);

    auto& archetype = getArchetype(mask, prev_archetype.sharedComponentInfo());
    if (&archetype == &prev_archetype) {
        return;
    }

    const auto prev_index = location.index;
    archetype.externalMove(entity, prev_archetype, prev_index, ComponentIdMask{});
}

bool EntityManager::removeSharedComponent(Entity entity, SharedComponentId component) {
    MUSTACHE_PROFILER_BLOCK_LVL_1(__FUNCTION__ );

    const auto& location = locations_[entity.id()];
    if (location.archetype.isNull()) {
            return false;
    }

    auto& prev_archetype = *archetypes_[location.archetype];

    if (!prev_archetype.hasComponent(component)) {
        return false;
    }

    auto shared_components_info = prev_archetype.sharedComponentInfo();
    shared_components_info.remove(component);

    auto& archetype = getArchetype(prev_archetype.componentMask(), shared_components_info);
    if (&archetype == &prev_archetype) {
        return false;
    }

    const auto prev_index = location.index;
    archetype.externalMove(entity, prev_archetype, prev_index, ComponentIdMask{});

    return true;
}

Archetype* EntityManager::getArchetypeOf(Entity entity) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );

    if (!isEntityValid(entity)) {
        return nullptr;
    }
    const auto archetype_index = locations_[entity.id()].archetype;
    return archetypes_[archetype_index].get();
}

void EntityManager::markDirty(Entity entity, ComponentId component_id) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );

    if (isEntityValid(entity)) {
        const auto location = locations_[entity.id()];
        const auto arch = archetypes_[location.archetype];
        const auto component_index = arch->getComponentIndex(component_id);
        if (component_index.isValid()) {
            arch->markComponentDirty(component_index, location.index, worldVersion());
        }
    }
}

void EntityManager::onLock() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    const auto thread_count = world_.dispatcher().maxThreadCount();
    temporal_storages_.resize(thread_count);
    next_entity_id_ = static_cast<uint32_t >(entities_.size());
}

void EntityManager::onUnlock() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__ );

    if (isLocked()) {
        throw std::runtime_error("Entity manager must be unlocked");
    }

    const auto doAction = [this](TemporalStorage& storage,
            const TemporalStorage::ActionInfo& info) {
        MUSTACHE_PROFILER_BLOCK_LVL_3("do single action");
        const auto entity = info.entity;
        switch (info.action) {
            case TemporalStorage::Action::kDestroyEntityNow:
                destroyNow(entity);
                break;
            case TemporalStorage::Action::kCreateEntity:
            {
                const auto index = entity.id().toInt();
                if (locations_.size() <= index) {
                    locations_.resize(index + 1);
                }
                if (entities_.size() <= index) {
                    entities_.resize(index + 1);
                }
                entities_[entity.id()] = entity;
                auto* archetype = storage.commands_.create[info.index];
                if (archetype) {
                    updateLocation(entity, archetype->id(), archetype->insert(entity));
                }
            }
                break;
            case TemporalStorage::Action::kDestroyEntity:
                destroy(entity);
                break;
            case TemporalStorage::Action::kRemoveComponent:
                removeComponent(entity, storage.commands_.remove[info.index].component_id);
                break;
            case TemporalStorage::Action::kAssignComponent:
            {
                auto& assign_info = storage.commands_.assign[info.index];
                const auto& component_info = *assign_info.type_info;
                auto dest = assign(entity, assign_info.component_id, false);
                auto src = assign_info.ptr;
                component_info.functions.move(dest, src);
                if (component_info.functions.destroy) {
                    component_info.functions.destroy(src);
                }
                assign_info.type_info = nullptr;
            }
                break;
        }
    };
    const auto apply_pack = [&doAction, this](TemporalStorage& storage,
            size_t begin, size_t end) {
        MUSTACHE_PROFILER_BLOCK_LVL_2("apply action pack");
        const bool can_be_optimized = (end - begin) > 1u &&
                storage.actions_[begin].action != TemporalStorage::Action::kDestroyEntityNow;
        if (!can_be_optimized) {
            doAction(storage, storage.actions_[begin]);
            return;
        }
        auto entity = storage.actions_[begin].entity;
        EntityLocationInWorld prev_location;
        Archetype* prev_archetype = nullptr;
        ComponentIdMask mask;
        if (storage.actions_[begin].action != TemporalStorage::Action::kCreateEntity) {
            prev_location = locations_[entity.id()];
            prev_archetype = &getArchetype(prev_location.archetype);
            mask = prev_archetype->componentMask();
        }
        size_t assign_begin = end;
        // update component mask to find new archetype
        for (size_t i = begin; i < end; ++i) {
            const auto info = storage.actions_[i];
            switch (info.action) {
                case TemporalStorage::Action::kDestroyEntityNow:
                    std::terminate(); // unreachable
                    break;
                case TemporalStorage::Action::kCreateEntity:
                    doAction(storage, info);
                    break;
                case TemporalStorage::Action::kDestroyEntity:
                    doAction(storage, info);
                    break;
                case TemporalStorage::Action::kRemoveComponent:
                    mask.set(storage.commands_.remove[info.index].component_id, false);
                    break;
                case TemporalStorage::Action::kAssignComponent:
                    mask.set(storage.commands_.assign[info.index].component_id, true);
                    assign_begin = std::min(assign_begin, i);
                    break;
            }
        }
        // Move to new archetype
        Archetype* archetype = nullptr;
        if (prev_archetype != nullptr) {
            archetype = &getArchetype(mask, prev_archetype->sharedComponentInfo());
            archetype->externalMove(entity, *prev_archetype, prev_location.index, mask);
        } else {
            archetype = &getArchetype(mask, {});
            archetype->insert(entity, mask);
        }

        if (assign_begin < end) {
            // Init new components
            const auto index = locations_[entity.id()].index;
            auto view = archetype->getElementView(index);
            for (size_t i = assign_begin; i < end; ++i) {
                auto& info = storage.commands_.assign[storage.actions_[i].index];
                const auto& component_info = ComponentFactory::componentInfo(info.component_id);
                const auto component_index = archetype->getComponentIndex(info.component_id);
                auto dest = view.getData(component_index);
                auto src = info.ptr;
                component_info.functions.move_constructor(dest, src);
                if (component_info.functions.destroy) {
                    component_info.functions.destroy(src);
                }
            }
        }
    };

    const auto apply_storage = [&apply_pack](TemporalStorage& storage) {
        MUSTACHE_PROFILER_BLOCK_LVL_2("merge storage");
        size_t begin = 0u;
        for (size_t i = 1; i < storage.actions_.size(); ++i) {
            const auto entity = storage.actions_[i].entity;
            const bool is_same = storage.actions_[i - 1].entity == entity;
            if (!is_same) {
                apply_pack(storage, begin, i);
                begin = i;
            }
        }
        apply_pack(storage, begin, storage.actions_.size());
    };

    const auto comparator = [](const auto& a, const auto& b) {
        if (a.entity != b.entity) {
            return a.entity.id() < b.entity.id();
        }
        return static_cast<uint32_t >(a.action) < static_cast<uint32_t >(b.action);
    };

    constexpr bool parallel_sort = true;
    if constexpr(parallel_sort) {
        MUSTACHE_PROFILER_BLOCK_LVL_1("Sort actions");
        world_.dispatcher().parallelFor([this, &comparator](size_t i){
            MUSTACHE_PROFILER_BLOCK_LVL_1("Sort storage");
            auto& storage = temporal_storages_[ThreadId::make(i)];
            std::sort(storage.actions_.begin(), storage.actions_.end(), comparator);
        }, 0u, temporal_storages_.size());
    } else {
        MUSTACHE_PROFILER_BLOCK_LVL_1("Sort actions");
        for (auto& storage : temporal_storages_) {
            std::sort(storage.actions_.begin(), storage.actions_.end(), comparator);
        }
    }

    for (auto& storage : temporal_storages_) {
        if (!storage.actions_.empty()) {
            apply_storage(storage);
            storage.clear();
        }
    }
}

[[nodiscard]] ThreadId EntityManager::threadId() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );
    return world_.dispatcher().currentThreadId();
}
