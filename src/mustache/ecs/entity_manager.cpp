#include "entity_manager.hpp"
#include <mustache/ecs/world.hpp>

using namespace mustache;

EntityManager::EntityManager(World& world):
        world_{world},
        this_world_id_{world.id()} {

}

Archetype& EntityManager::getArchetype(const ComponentMask& mask) {
    if (mask.isEmpty()) {
        // NOTE: it might make sense to create null value for the "empty" archetype
        throw std::runtime_error("Mask is empty");
    }
    auto& result = mask_to_arch_[mask];
    if(result) {
        return *result;
    }
    auto deleter = [this](Archetype* archetype) {
        clearArchetype(*archetype);
        delete archetype;
    };
    result = new Archetype(world_, archetypes_.back_index().next(), mask);
    archetypes_.emplace_back(result, deleter);
    return *result;
}

Entity EntityManager::create() {
    if(!empty_slots_) {
        Entity result{entities_.size()};
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
    if (marked_for_delete_.empty()) {
        return;
    }
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
