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

void Archetype::callDestructor(const ArchetypeComponentDataStorage::ElementView& view) {
    for (const auto &info : operation_helper_.destroy) {
        info.destructor(view.getData<FunctionSafety::kUnsafe>(info.component_index));
    }
    data_storage_.decrSize();
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

void Archetype::internalMove(ArchetypeEntityIndex source_index, ArchetypeEntityIndex destination_index) {
    // moving last entity to index
    ComponentIndex component_index = ComponentIndex::make(0);
    auto source_view = data_storage_.getElementView(ComponentStorageIndex::fromArchetypeIndex(source_index));
    auto dest_view = data_storage_.getElementView(ComponentStorageIndex::fromArchetypeIndex(destination_index));
    for (auto& info : operation_helper_.internal_move) {
        auto source_ptr = source_view.getData<FunctionSafety::kUnsafe>(component_index);
        auto dest_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
        info.move(dest_ptr, source_ptr);
        ++component_index;
    }

    auto source_entity = *source_view.getEntity<FunctionSafety::kUnsafe>();
    auto& dest_entity = *dest_view.getEntity<FunctionSafety::kUnsafe>();

    world_.entities().updateLocation(dest_entity, ArchetypeIndex::null(), ArchetypeEntityIndex::null());
    world_.entities().updateLocation(source_entity, id_, destination_index);

    dest_entity = source_entity;

    callDestructor(source_view);
}

void Archetype::remove(Entity entity_to_destroy, ArchetypeEntityIndex entity_index) {
//    Logger{}.debug("Removing entity from: %s pos: %d", mask_.toString(), entity_index.toInt());

    const auto last_index = data_storage_.lastItemIndex();
    const auto index = ComponentStorageIndex::fromArchetypeIndex(entity_index);
    if (index == last_index) {
        if (!operation_helper_.destroy.empty()) {
            callDestructor(data_storage_.getElementView(index));
        } else {
            data_storage_.decrSize();
        }
        world_.entities().updateLocation(entity_to_destroy, ArchetypeIndex::null(), ArchetypeEntityIndex::null());
    } else {
        // TODO: add test for this part
        internalMove(last_index.toArchetypeIndex(), entity_index);
    }
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
