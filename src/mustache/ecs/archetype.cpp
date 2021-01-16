#include "archetype.hpp"

#include <mustache/utils/logger.hpp>

#include <mustache/ecs/world.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/new_component_data_storage.hpp>
#include <mustache/ecs/default_component_data_storage.hpp>

using namespace mustache;

Archetype::Archetype(World& world, ArchetypeIndex id, const ComponentIdMask& mask,
                     const SharedComponentsInfo& shared_components_info, uint32_t chunk_size):
        world_{world},
        mask_{mask},
        shared_components_info_ {shared_components_info},
        operation_helper_{world.memoryManager(), mask},
        data_storage_{std::make_unique<DefaultComponentDataStorage>(mask, world_.memoryManager())},
        entities_{world.memoryManager()},
        components_count_{mask.componentsCount()},
        chunk_size_{chunk_size},
        chunk_versions_{world.memoryManager()},
        global_versions_{world.memoryManager()},
        id_{id} {
    global_versions_.resize(components_count_, WorldVersion::null());
    Logger{}.debug("Archetype version chunk size: %d", chunk_size);
}

Archetype::~Archetype() {
    if (!isEmpty()) {
        Logger{}.error("Destroying non-empty archetype");
    }
    data_storage_->clear(true);
}

void Archetype::callDestructor(const ElementView& view) {
    for (const auto &info : operation_helper_.destroy) {
        info.destructor(view.getData<FunctionSafety::kUnsafe>(info.component_index));
    }
    popBack();
}

void Archetype::externalMove(Entity entity, Archetype& prev_archetype, ArchetypeEntityIndex prev_index,
                             const ComponentIdMask& skip_constructor) {

    const auto index = pushBack(entity);

    ComponentIndex component_index = ComponentIndex::make(0);
    const auto source_view = prev_archetype.getElementView(prev_index);
    const auto dest_view = getElementView(index.toArchetypeIndex());
    for (const auto& info : operation_helper_.external_move) {
        auto prev_ptr = source_view.getData<FunctionSafety::kSafe>(prev_archetype.getComponentIndex<FunctionSafety::kSafe>(info.id));
        if (prev_ptr != nullptr) {
            auto component_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
            info.move(component_ptr, prev_ptr);
        } else if (info.constructor && !skip_constructor.has(info.id)) {
            auto component_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
            info.constructor(component_ptr);
        }
        ++component_index;
    }

    prev_archetype.remove(*source_view.getEntity<FunctionSafety::kUnsafe>(), prev_index);

    updateAllVersions(index.toArchetypeIndex(), worldVersion());
    world_.entities().updateLocation(entity, id_, index.toArchetypeIndex());
}

ArchetypeEntityIndex Archetype::insert(Entity entity, const ComponentIdMask& skip_constructor) {
    const auto index = pushBack(entity);

    const bool is_skip_mask_empty = skip_constructor.isEmpty();
    const bool skip_all_constructors = (skip_constructor == mask_);
    if (!skip_all_constructors) {
        const auto view = getElementView(index.toArchetypeIndex());
        for (const auto& info : operation_helper_.insert) {
            const auto& component_id = operation_helper_.component_index_to_component_id[info.component_index];
            if (is_skip_mask_empty || !skip_constructor.has(component_id)) {
                auto component_ptr = view.getData<FunctionSafety::kUnsafe>(info.component_index);
                info.constructor(component_ptr);
            }
        }
    }
    updateAllVersions(index.toArchetypeIndex(), worldVersion());
    world_.entities().updateLocation(entity, id_, index.toArchetypeIndex());
    return index.toArchetypeIndex();
}

void Archetype::internalMove(ArchetypeEntityIndex source_index, ArchetypeEntityIndex destination_index) {
    // moving last entity to index
    ComponentIndex component_index = ComponentIndex::make(0);
    auto source_view = getElementView(source_index);
    auto dest_view = getElementView(destination_index);
    for (auto& info : operation_helper_.internal_move) {
        auto source_ptr = source_view.getData<FunctionSafety::kUnsafe>(component_index);
        auto dest_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
        info.move(dest_ptr, source_ptr);
        ++component_index;
    }

    auto source_entity = *source_view.getEntity<FunctionSafety::kUnsafe>();
    auto& dest_entity = *dest_view.getEntity<FunctionSafety::kUnsafe>();

    const auto world_version = worldVersion();
    updateAllVersions(source_index, world_version);
    updateAllVersions(destination_index, world_version);

    world_.entities().updateLocation(dest_entity, ArchetypeIndex::null(), ArchetypeEntityIndex::null());
    world_.entities().updateLocation(source_entity, id_, destination_index);

    dest_entity = source_entity;

    callDestructor(source_view);
}

void Archetype::remove(Entity entity_to_destroy, ArchetypeEntityIndex entity_index) {
//    Logger{}.debug("Removing entity from: %s pos: %d", mask_.toString(), entity_index.toInt());

    const auto last_index = data_storage_->lastItemIndex().toArchetypeIndex();
    if (entity_index == last_index) {
        if (!operation_helper_.destroy.empty()) {
            callDestructor(getElementView(entity_index));
        } else {
            popBack();
        }
        updateAllVersions(entity_index, worldVersion());
        world_.entities().updateLocation(entity_to_destroy, ArchetypeIndex::null(), ArchetypeEntityIndex::null());
    } else {
        internalMove(last_index, entity_index);
    }
}

WorldVersion Archetype::worldVersion() const noexcept {
    return world_.version();
}

void Archetype::clear() {
    if (isEmpty()) {
        return;
    }

    for (const auto& info : operation_helper_.destroy) {
        for (auto i = ComponentStorageIndex::make(0); i < data_storage_->lastItemIndex().next(); ++i) {
            auto component_ptr = data_storage_->getData(info.component_index, i);
            info.destructor(component_ptr);
        }
    }

    entities_.clear();
    data_storage_->clear(false);
}
