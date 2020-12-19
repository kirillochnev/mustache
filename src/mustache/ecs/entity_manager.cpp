#include "entity_manager.hpp"
#include <mustache/ecs/world.hpp>

using namespace mustache;

EntityManager::EntityManager(World& world):
        world_{world},
        entities_{world.memoryManager()},
        locations_{world.memoryManager()},
        marked_for_delete_{world.memoryManager()},
        this_world_id_{world.id()},
        world_version_{world.version()},
        archetypes_{world.memoryManager()} {

}

Archetype& EntityManager::getArchetype(const ComponentIdMask& mask) {
    const ComponentIdMask arch_mask = mask.merge(getExtraComponents(mask));
    auto& result = mask_to_arch_[arch_mask];
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

    result = new Archetype(world_, archetypes_.back_index().next(), arch_mask, chunk_size);
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
    archetype.forEachEntity([this](Entity entity) {
        const auto id = entity.id();
        auto& location = locations_[id];
        location.archetype = ArchetypeIndex::null();
        entities_[id].reset(empty_slots_ ? next_slot_ : id.next(), entity.version().next());
        next_slot_ = id;
        ++empty_slots_;
    });
    archetype.clear();
}

void EntityManager::addDependency(ComponentId component, const ComponentIdMask& extra) noexcept {
    auto& dependency = dependencies_[component];
    dependency = dependency.merge(extra.merge(getExtraComponents(extra)));
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

