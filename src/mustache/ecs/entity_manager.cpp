#include "entity_manager.hpp"
#include <mustache/ecs/world.hpp>

using namespace mustache;

EntityManager::EntityManager(World& world):
        world_{world},
        this_world_id_{world.id()} {

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
    const auto version = entities_[id.toInt()].version();
    next_slot_ = entities_[id.toInt()].id();
    entity.reset(id, version);
    entities_[id.toInt()] = entity;
    locations_[id.toInt()] = EntityLocationInWorld{};
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
        auto& location = locations_[id.toInt()];
        location.archetype = ArchetypeIndex::null();
        entities_[id.toInt()].reset(empty_slots_ ? next_slot_ : id.next(), entity.version().next());
        next_slot_ = id;
        ++empty_slots_;
    });
    archetype.clear();
}
