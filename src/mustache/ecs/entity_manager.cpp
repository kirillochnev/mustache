#include "entity_manager.hpp"

using namespace mustache;

EntityManager::EntityManager(World& world):
        world_{world} {

}

Archetype& EntityManager::getArchetype(const ComponentMask& mask) {
    auto& result = mask_to_arch_[mask];
    if(result) {
        return *result;
    }
    auto unique_ptr = std::make_unique<Archetype>(world_, ArchetypeIndex::make(archetypes_.size()), mask);
    result = unique_ptr.get();
    archetypes_.emplace_back(std::move(unique_ptr));
    return *result;
}

Entity EntityManager::create() {
    if(!empty_slots_) {
        Entity result{entities_.size()};
        entities_.push_back(result);
        locations_.push_back({});
        return result;
    }

    Entity entity;
    const auto id = next_slot_;
    const auto version = entities_[id].version();
    next_slot_ = entities_[id].id().toInt();
    entity.reset(EntityId::make(id), version);
    entities_[id] = entity;
    locations_[id] = EntityLocationInWorld{};
    --empty_slots_;

    return entity;
}

void EntityManager::clear() {
    entities_.clear();
    locations_.clear();
    next_slot_ = 0u;
    empty_slots_ = 0u;
    for(auto& arh : archetypes_) {
        arh->clear();
    }
}