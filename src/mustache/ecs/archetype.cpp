#include "archetype.hpp"
#include <mustache/utils/logger.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/world.hpp>

using namespace mustache;

Archetype::Archetype(World& world, ArchetypeIndex id, const ComponentIdMask& mask):
    world_{world},
    mask_{mask},
    operation_helper_{mask},
    data_storage_{mask},
    id_{id} {
}

Archetype::~Archetype() {
    if (!data_storage_.isEmpty()) {
        Logger{}.error("Destroying non-empty archetype");
    }
    data_storage_.clear(true);
}

void Archetype::externalMove(Entity entity, Archetype& prev_archetype, ArchetypeEntityIndex prev_index,
        bool initialize_missing_components) {


    entities_.push_back(entity);
    const auto index = data_storage_.pushBackAndUpdateVersion(entity, worldVersion());
//    Logger{}.debug("Moving entity from: %s pos: %d,  to: %s pos: %d",
//            prev_archetype.mask_.toString(), prev_index.toInt(), mask_.toString(), index.toInt());

    ComponentIndex component_index = ComponentIndex::make(0);
    const auto source_view = prev_archetype.data_storage_.getElementView(ComponentStorageIndex::fromArchetypeIndex(prev_index));
    const auto dest_view = data_storage_.getElementView(index);
    for (const auto& info : operation_helper_.external_move) {
        auto prev_ptr = source_view.getData<FunctionSafety::kSafe>(prev_archetype.getComponentIndex<FunctionSafety::kSafe>(info.id));
        if (prev_ptr != nullptr) {
            auto component_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
            info.move(component_ptr, prev_ptr);
        } else if (initialize_missing_components && info.constructor) {
            auto component_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
            info.constructor(component_ptr);
        }
        ++component_index;
    }

    // TODO: use callback to update entity location for prev archetype!
    prev_archetype.remove(*source_view.getEntity<FunctionSafety::kUnsafe>(), prev_index);

    world_.entities().updateLocation(entity, id_, index.toArchetypeIndex());
}

ArchetypeEntityIndex Archetype::insert(Entity entity, bool call_constructor) {
    const auto index = data_storage_.pushBackAndUpdateVersion(entity, worldVersion());
    entities_.push_back(entity);

    if (call_constructor) {
        const auto view = data_storage_.getElementView(index);
        for (const auto &info : operation_helper_.insert) {
            auto component_ptr = view.getData<FunctionSafety::kUnsafe>(info.component_index);
            info.constructor(component_ptr);
        }
    }
    world_.entities().updateLocation(entity, id_, index.toArchetypeIndex());
    return index.toArchetypeIndex();
}

void Archetype::remove(Entity entity_to_destroy, ArchetypeEntityIndex entity_index) {
//    Logger{}.debug("Removing entity from: %s pos: %d", mask_.toString(), index.toInt());
    const auto call_destructors = [this, &entity_to_destroy](ComponentStorageIndex destroy_index) {
        if (!operation_helper_.destroy.empty()) {
            auto view = data_storage_.getElementView(destroy_index);
            for (const auto &info : operation_helper_.destroy) {
                info.destructor(view.getData<FunctionSafety::kUnsafe>(info.component_index));
            }
        }
        data_storage_.decrSize();
        world_.entities().updateLocation(entity_to_destroy, ArchetypeIndex::null(), ArchetypeEntityIndex::null());
    };

    const auto last_index = data_storage_.lastItemIndex();
    const auto index = ComponentStorageIndex::fromArchetypeIndex(entity_index);
    if (index == last_index) {
        call_destructors(last_index);
        return;
    }
    // TODO: add test for this part

    // moving last entity to index
    ComponentIndex component_index = ComponentIndex::make(0);
    auto last_item_view = data_storage_.getElementView(last_index);
    auto dest_view = data_storage_.getElementView(index);
    for (auto& info : operation_helper_.internal_move) {
        auto source_ptr = last_item_view.getData<FunctionSafety::kUnsafe>(component_index);
        auto dest_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
        info.move(dest_ptr, source_ptr);
    }

    auto last_entity = *last_item_view.getEntity<FunctionSafety::kUnsafe>();
    *dest_view.getEntity<FunctionSafety::kUnsafe>() = last_entity;
    call_destructors(last_index);

    world_.entities().updateLocation(last_entity, id_, entity_index);
}

WorldVersion Archetype::worldVersion() const noexcept {
    return world_.version();
}

void Archetype::clear() {
    if (data_storage_.isEmpty()) {
        return;
    }

    for (const auto& info : operation_helper_.destroy) {
        for (auto i = ComponentStorageIndex::make(0); i < data_storage_.lastItemIndex().next(); ++i) {
            auto component_ptr = data_storage_.getData(info.component_index, i);
            info.destructor(component_ptr);
        }
    }

    data_storage_.clear(false);
}
