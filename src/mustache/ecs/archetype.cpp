#include "archetype.hpp"
#include <mustache/utils/logger.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/world.hpp>
#include <cstdlib>

using namespace mustache;

Archetype::Archetype(World& world, ArchetypeIndex id, const ComponentMask& mask):
    world_{world},
    mask_{mask},
    operation_helper_{mask},
    data_storage_{mask},
    id_{id} {
    uint32_t i = 0;
    for (auto component_id : mask.components()) {
        const auto& info = ComponentFactory::componentInfo(component_id);
        name_ += " [" + info.name + "]";
        Logger{}.debug("Offset of: %s = %d", info.name, data_storage_.component_getter_info_[ComponentIndex::make(i++)].offset.toInt());
    }
    Logger{}.debug("New archetype created, components: %s | chunk size: %d",
                   name_.c_str(), operation_helper_.chunkCapacity());

    // TODO: remove
    if (data_storage_.chunk_capacity_ != ChunkCapacity::make(operation_helper_.chunkCapacity())) {
        throw std::runtime_error(std::to_string(data_storage_.chunk_capacity_.toInt()) + " vs " + std::to_string(operation_helper_.chunkCapacity()));
    }
//    data_storage_.chunk_capacity_ = ChunkCapacity::make(operation_helper_.chunkCapacity());
}

Archetype::~Archetype() {
    if (!data_storage_.isEmpty()) {
        Logger{}.error("Destroying non-empty archetype");
    }
    data_storage_.clear(true);
}

ArchetypeEntityIndex Archetype::insert(Entity entity, Archetype& prev_archetype, ArchetypeEntityIndex prev_index,
        bool initialize_missing_components) {

    const auto index = data_storage_.pushBackAndUpdateVersion(entity, worldVersion());

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
    prev_archetype.remove(prev_index);

    return index.toArchetypeIndex();
}

ArchetypeEntityIndex Archetype::insert(Entity entity, bool call_constructor) {
    const auto index = data_storage_.pushBackAndUpdateVersion(entity, worldVersion());

    if (call_constructor) {
        const auto view = data_storage_.getElementView(index);
        for (const auto &info : operation_helper_.insert) {
            auto component_ptr = view.getData<FunctionSafety::kUnsafe>(info.component_index);
            info.constructor(component_ptr);
        }
    }

    return index.toArchetypeIndex();
}

Entity Archetype::remove(ArchetypeEntityIndex index) {
    const auto call_destructors = [this](ComponentStorageIndex destroy_index) {
        for (const auto& info : operation_helper_.destroy) {
            auto component_ptr = data_storage_.getData(info.component_index, destroy_index);
            info.destructor(component_ptr);
        }
        data_storage_.decrSize();
    };

    const auto location = entityIndexToInternalLocation(index);
    const auto source_index = data_storage_.lastItemIndex();
    const auto dest_index = ComponentStorageIndex::fromArchetypeIndex(index);
    if (dest_index == source_index) {
        call_destructors(source_index);
        return Entity{};
    }
    // TODO: add test for this part

    // moving last entity to index
    ComponentIndex component_index = ComponentIndex::make(0);
    auto source_view = data_storage_.getElementView(source_index);
    auto dest_view = data_storage_.getElementView(dest_index);
    for (auto& info : operation_helper_.internal_move) {
        auto source_ptr = source_view.getData<FunctionSafety::kUnsafe>(component_index);
        auto dest_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
        info.move(dest_ptr, source_ptr);
    }

    auto source_entity = *data_storage_.getEntityData<FunctionSafety::kUnsafe>(source_index);
    *operation_helper_.getEntity<FunctionSafety::kUnsafe>(location) = source_entity;
    call_destructors(source_index);

    return source_entity;
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
