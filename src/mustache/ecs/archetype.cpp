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
    const auto index = data_storage_.pushBackAndUpdateVersion(entity, worldVersion()).toArchetypeIndex();
    const auto location = entityIndexToInternalLocation<FunctionSafety::kUnsafe>(index);

    const auto source_location = prev_archetype.entityIndexToInternalLocation(prev_index);
    for (const auto& info : operation_helper_.external_move) {
        const auto prev_offset = prev_archetype.getComponentOffset(info.id).add(source_location.index.toInt() * info.size);
        if (!prev_offset.isNull()) {
            auto prev_ptr = source_location.chunk->dataPointerWithOffset(prev_offset);
            const auto component_offset = info.offset.add(location.index.toInt() * info.size);
            auto component_ptr = location.chunk->dataPointerWithOffset(component_offset);
            info.move(component_ptr, prev_ptr);
            continue;
        }
        if (initialize_missing_components && info.constructor) {
            const auto component_offset = info.offset.add(location.index.toInt() * info.size);
            auto component_ptr = location.chunk->dataPointerWithOffset(component_offset);
            info.constructor(component_ptr);
        }
    }
    // TODO: use callback to update entity location for prev archetype!
    prev_archetype.remove(prev_index);

    return index;
}

ArchetypeEntityIndex Archetype::insert(Entity entity, bool call_constructor) {
    const auto index = data_storage_.pushBackAndUpdateVersion(entity, worldVersion()).toArchetypeIndex();
    const auto location = entityIndexToInternalLocation<FunctionSafety::kUnsafe>(index);

    if (call_constructor) {
        for (const auto &info : operation_helper_.insert) {
            const auto component_offset = info.offset.add(location.index.toInt() * info.component_size);
            auto component_ptr = location.chunk->dataPointerWithOffset(component_offset);
            info.constructor(component_ptr);
        }
    }
    return index;
}

Entity Archetype::remove(ArchetypeEntityIndex index) {
    const auto call_destructors = [this](const ArchetypeInternalEntityLocation& location) {
        for (const auto& info : operation_helper_.destroy) {
            const auto component_offset = info.offset.add(location.index.toInt() * info.component_size);
            auto component_ptr = location.chunk->dataPointerWithOffset(component_offset);
            info.destructor(component_ptr);
        }
        data_storage_.decrSize();
    };

    const auto location = entityIndexToInternalLocation(index);
    const auto last_item_index = ArchetypeEntityIndex::make(data_storage_.size() - 1);
    if (index == last_item_index) {
        call_destructors(location);
        return Entity{};
    }
    // moving last entity to index
    const auto source_location = entityIndexToInternalLocation(ArchetypeEntityIndex::make(data_storage_.size() - 1));
    for (auto& info : operation_helper_.internal_move) {
        const auto source_component_offset = info.offset.add(source_location.index.toInt() * info.component_size);
        const auto dest_component_offset = info.offset.add(location.index.toInt() * info.component_size);
        auto source_ptr = location.chunk->dataPointerWithOffset(source_component_offset);
        auto dest_ptr = location.chunk->dataPointerWithOffset(dest_component_offset);
        info.move(dest_ptr, source_ptr);
    }
    auto entity_ptr = operation_helper_.getEntity(source_location);
    if (entity_ptr == nullptr) {
        entity_ptr = operation_helper_.getEntity(source_location);
    }
    auto source_entity = *entity_ptr;
    *operation_helper_.getEntity(location) = source_entity;
    call_destructors(source_location);
    return source_entity;
}

WorldVersion Archetype::worldVersion() const noexcept {
    return world_.version();
}

void Archetype::clear() {
    if (data_storage_.isEmpty()) {
        return;
    }

    const auto for_each_location = [this, size = data_storage_.size()](auto&& f) {
        const auto chunk_last_index = operation_helper_.index_of_last_entity_in_chunk;
        auto num_items = size;
        ArchetypeInternalEntityLocation location;
        for (auto chunk : data_storage_.chunks_) {
            location.chunk = chunk;
            for (location.index = ChunkEntityIndex::make(0); location.index <= chunk_last_index; ++location.index) {
                f(location);
                if (--num_items == 0) {
                    return;
                }
            }
        }
    };
    for (const auto& info : operation_helper_.destroy) {
        for_each_location([&info, this](const auto& location){
            const auto component_offset = info.offset.add(location.index.toInt() * info.component_size);
            auto component_ptr = location.chunk->dataPointerWithOffset(component_offset);
            info.destructor(component_ptr);
        });
    }
    data_storage_.clear(false);
}
