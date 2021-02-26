#include "entity_manager.hpp"
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

}

Archetype& EntityManager::getArchetype(const ComponentIdMask& mask, const SharedComponentsInfo& shared) {
    const ComponentIdMask arch_mask = mask.merge(getExtraComponents(mask));
    ArchetypeComponents archetype_components;
    archetype_components.unique = arch_mask;
    archetype_components.shared = shared.mask();
    auto& map = mask_to_arch_[archetype_components];
    auto& result = map[shared.data()];
    if(result) {
        return *result;
    }
    auto deleter = [this](Archetype* archetype) {
        clearArchetype(*archetype);
        delete archetype;
    };

    auto min = archetype_chunk_size_info_.min_size;
    auto max = archetype_chunk_size_info_.max_size;

    for (const auto& func : get_chunk_size_functions_) {
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
        throw std::runtime_error("Can not create archetype: " + std::to_string(max) + " < " + std::to_string(min));
    }

    if (chunk_size < min) {
        chunk_size = min;
    }

    if (max > 0 && chunk_size > max) {
        chunk_size = max;
    }

    result = new Archetype(world_, archetypes_.back_index().next(), arch_mask, shared, chunk_size);
    archetypes_.emplace_back(result, deleter);
    return *result;
}

Entity EntityManager::create() {
    const auto get_entity = [this] {
        if(!empty_slots_) {
            const auto id = EntityId::make(entities_.size());
            const auto version = EntityVersion::make(0);
            const auto world_id = this_world_id_;
            Entity result{id, version, world_id};
            entities_.push_back(result);
            locations_.emplace_back();
            return result;
        }

        Entity entity;
        const auto id = next_slot_;
        const auto version = entities_[id].version();
        next_slot_ = entities_[id].id();
        entity.reset(id, version, this_world_id_);
        entities_[id] = entity;
        locations_[id] = EntityLocationInWorld{};
        --empty_slots_;

        return entity;
    };
    auto entity = get_entity();
    getArchetype<>().insert(entity);
    return entity;
}

void EntityManager::clear() {
    entities_.clear();
    locations_.clear();
    next_slot_ = EntityId::make(0);
    empty_slots_ = 0u;
    for(auto& arh : archetypes_) {
        arh->clear();
    }
}

void EntityManager::update() {
    world_version_ = world_.version();
    for (auto entity : marked_for_delete_) {
        destroyNow(entity);
    }
    marked_for_delete_.clear();
}

void EntityManager::clearArchetype(Archetype& archetype) {
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
    auto& dependency = dependencies_[component];
    dependency = dependency.merge(extra.merge(getExtraComponents(extra)));
}

void EntityManager::addChunkSizeFunction(const ArchetypeChunkSizeFunction& function) {
    if (function) {
        /// TODO: update archetypes
        get_chunk_size_functions_.push_back(function);
    }
}

void EntityManager::setDefaultArchetypeVersionChunkSize(uint32_t value) noexcept {
    /// TODO: update archetypes
    archetype_chunk_size_info_.default_size = value;
}

ComponentIdMask EntityManager::getExtraComponents(const ComponentIdMask& mask) const noexcept {
    ComponentIdMask result;
    if (!dependencies_.empty()) {
        mask.forEachItem([&result, this](ComponentId id) {
            const auto find_res = dependencies_.find(id);
            if (find_res != dependencies_.end()) {
                result = result.merge(find_res->second);
            }
        });
    }
    return result;
}

SharedComponentPtr EntityManager::getCreatedSharedComponent(const SharedComponentPtr& ptr, SharedComponentId id) {
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

bool EntityManager::removeComponent(Entity entity, ComponentId component) {
    const auto& location = locations_[entity.id()];
    if (location.archetype.isNull()) {
        return false;
    }

    auto& prev_archetype = *archetypes_[location.archetype];
    if (!prev_archetype.hasComponent(component)) {
        return false;
    }

    auto mask = prev_archetype.mask_;
    mask.set(component, false);

    auto& archetype = getArchetype(mask, prev_archetype.sharedComponentInfo());
    if (&archetype == &prev_archetype) {
        return false;
    }

    const auto prev_index = location.index;
    archetype.externalMove(entity, prev_archetype, prev_index, ComponentIdMask{});

    return true;
}

bool EntityManager::removeSharedComponent(Entity entity, SharedComponentId component) {
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
    if (!isEntityValid(entity)) {
        return nullptr;
    }
    const auto archetype_index = locations_[entity.id()].archetype;
    return archetypes_[archetype_index].get();
}

void EntityManager::markDirty(Entity entity, ComponentId component_id) noexcept {
    if (isEntityValid(entity)) {
        const auto location = locations_[entity.id()];
        const auto arch = archetypes_[location.archetype];
        const auto component_index = arch->getComponentIndex(component_id);
        if (component_index.isValid()) {
            arch->markComponentDirty(component_index, location.index, worldVersion());
        }
    }
}
