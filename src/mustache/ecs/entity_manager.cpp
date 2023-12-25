#include "entity_manager.hpp"

#include <mustache/utils/profiler.hpp>

#include <mustache/ecs/world.hpp>

using namespace mustache;

namespace mustache {
    bool operator<(const ArchetypeComponents& lhs, const ArchetypeComponents& rhs) noexcept {
        return memcmp(&lhs.unique, &rhs.unique, sizeof(rhs.unique)) < 0;// lhs.unique < rhs.unique;
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
        if (v.get() == ptr.get() || ComponentFactory::instance().isEq(v.get(), ptr.get(), id)) {
            return v;
        }
    }
    arr.push_back(ptr);
    return ptr;
}

void EntityManager::removeComponent(Entity entity, ComponentId component) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__ );

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
    archetype.externalMove(entity, prev_archetype, prev_index, ComponentIdMask::null());
}

bool EntityManager::removeSharedComponent(Entity entity, SharedComponentId component) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__ );

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
    archetype.externalMove(entity, prev_archetype, prev_index, ComponentIdMask::null());

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
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
    if (isLocked()) {
        throw std::runtime_error("Entity manager must be unlocked");
    }

    for (auto& storage : temporal_storages_) {
        applyStorage(storage);
        storage.clear();
    }
}

void EntityManager::applyCommandPack(TemporalStorage& storage, size_t begin, size_t end) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);

    ComponentIdMask final_mask;
    ComponentIdMask initial_mask;
    SharedComponentsInfo shared;
    const auto entity = storage.actions_[begin].entity;
    const bool create = storage.actions_[begin].action == TemporalStorage::Action::kCreateEntity;
    if (create) {
        const auto index = storage.actions_[begin].create_action_index;
        if (storage.create_actions_.has(index)) {
            final_mask = storage.create_actions_[index].mask;
            shared = storage.create_actions_[index].shared;
        }
        if (!entities_.has(entity.id())) {
            entities_.resize(entity.id().next().toInt());
            locations_.resize(entity.id().next().toInt());
            entities_[entity.id()] = entity;
        }
    }
    else {
        auto archetype = getArchetypeOf(entity);
        final_mask = archetype->componentMask();
        shared = archetype->sharedComponentInfo();
    }
    initial_mask = final_mask;

    for (size_t i = create ? begin + 1 : begin; i < end; ++i) {
        const auto& command = storage.actions_[i];
        switch (command.action) {
        case TemporalStorage::Action::kDestroyEntityNow:
            if (create) {
                releaseEntityIdUnsafe(entity);
            }
            else {
                destroyNow(entity);
            }
            return;
        case TemporalStorage::Action::kCreateEntity:
            throw std::runtime_error("Create command should be first command for entity: " + std::to_string(entity.id().toInt()));
            break;
        case TemporalStorage::Action::kDestroyEntity:
            destroy(command.entity);
            break;
        case TemporalStorage::Action::kRemoveComponent:
            final_mask.set(command.component_id, false);
            break;
        case TemporalStorage::Action::kAssignComponent:
            final_mask.set(command.component_id, true);
            break;
        default:
            break;
        }
    }

    Archetype& archetype = getArchetype(final_mask, shared);
    if (create) {
        archetype.insert(entity, initial_mask.inverse());
    }
    else if (initial_mask != final_mask) {
        const auto location = locations_[entity.id()];
        archetype.externalMove(entity, getArchetype(location.archetype), location.index, final_mask);
    }

    auto view = archetype.getElementView(locations_[entity.id()].index);
    for (size_t i = begin; i < end; ++i) {
        const auto& command = storage.actions_[i];
        if (command.action != TemporalStorage::Action::kAssignComponent) {
            continue;
        }
        auto dest = view.getData(archetype.getComponentIndex(command.component_id));
        const auto& component_functions = ComponentFactory::instance().componentInfo(command.component_id).functions;
        component_functions.move_constructor(dest, command.ptr);
        if (component_functions.after_assign) {
            component_functions.after_assign(dest, command.entity, world_);
        }
    }
}

void EntityManager::applyCommandPackUnoptimized(TemporalStorage& storage, size_t begin, size_t end) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);

    for (size_t i = begin; i < end; ++i) {
        const auto& command = storage.actions_[i];
        switch (command.action) {
        case TemporalStorage::Action::kDestroyEntityNow:
            destroyNow(command.entity);
            break;
        case TemporalStorage::Action::kCreateEntity:
        {
            const auto index = command.entity.id().toInt();
            if (locations_.size() <= index) {
                locations_.resize(index + 1);
            }
            if (entities_.size() <= index) {
                entities_.resize(index + 1);
            }
            entities_[command.entity.id()] = command.entity;
            if (storage.create_actions_.has(command.create_action_index)) {
                const auto& [mask, shared] = storage.create_actions_[command.create_action_index];
                getArchetype(mask, shared).insert(command.entity);
            }
            else {
                getArchetype<>().insert(command.entity);
            }
        }
        break;
        case TemporalStorage::Action::kDestroyEntity:
            destroy(command.entity);
            break;
        case TemporalStorage::Action::kRemoveComponent:
            removeComponent(command.entity, command.component_id);
            break;
        case TemporalStorage::Action::kAssignComponent:
            command.type_info->functions.move(assign(command.entity, command.component_id), command.ptr);
            break;
        default:
            break;
        }
    }
}

void EntityManager::applyStorage(TemporalStorage& storage) { // optimized version
    // TODO: this version iterates over commands 3 times, it is possible to merge 1-st and 2-nd iterations.
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);

    if (storage.actions_.empty()) {
        return;
    }

    size_t begin = 0;
    size_t end = begin + 1;
    Entity prev_entity = storage.actions_[begin].entity;
    for (; end < storage.actions_.size(); ++end) {
        const auto& action = storage.actions_[end];
        if (action.entity == prev_entity) {
            continue;
        }
        applyCommandPack(storage, begin, end);
        begin = end;
        prev_entity = storage.actions_[end].entity;
    }
    applyCommandPack(storage, begin, end);

    for (auto& command : storage.actions_) {
        if (command.action == TemporalStorage::Action::kAssignComponent) {
            const auto& info = ComponentFactory::instance().componentInfo(command.component_id);
            if (info.functions.destroy) {
                info.functions.destroy(command.ptr);
            }
        }
    }
}

[[nodiscard]] ThreadId EntityManager::threadId() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__ );
    return world_.dispatcher().currentThreadId();
}
