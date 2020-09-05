#include "archetype.hpp"
#include <mustache/utils/logger.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/world.hpp>

using namespace mustache;

namespace {
    MUSTACHE_INLINE void updateVersion(uint32_t version, uint32_t num_components,
            ComponentOffset version_offset, Chunk* chunk) {
        auto version_ptr = chunk->dataPointerWithOffset<uint32_t>(version_offset);
        for (uint32_t i = 0; i < num_components; ++i) {
            version_ptr[i] = version;
        }
    }
}
Archetype::Archetype(World& world, ArchetypeIndex id, const ComponentMask& mask):
    world_{world},
    mask_{mask},
    operation_helper_{mask},
    id_{id} {
    uint32_t i = 0;
    for (auto component_id : mask.components()) {
        const auto& info = ComponentFactory::componentInfo(component_id);
        name_ += " [" + info.name + "]";
        Logger{}.debug("Offset of: %s = %d", info.name, operation_helper_.get[i++].offset.toInt());
    }
    Logger{}.debug("New archetype created, components: %s | chunk size: %d",
                   name_.c_str(), operation_helper_.capacity);
}

Archetype::~Archetype() {
    for(auto chunk : chunks_) {
        freeChunk(chunk);
    }
}

ArchetypeEntityIndex Archetype::insert(Entity entity) {
    reserve(size() + 1);
    const auto index = ArchetypeEntityIndex::make(size_++);
    // the item at this index has not been created yet,
    // but memory has already been allocated, so an unsafe version of entityIndexToInternalLocation must be used
    const auto location = entityIndexToInternalLocation<FunctionSafety::kUnsafe>(index);
    updateVersion(world_.version(), operation_helper_.num_components,
            operation_helper_.version_offset, location.chunk);
    const auto entity_offset = operation_helper_.entity_offset.add(location.index.toInt() * sizeof(Entity));
    new (location.chunk->dataPointerWithOffset<Entity>(entity_offset)) Entity (entity);
    for (const auto& info : operation_helper_.insert) {
        const auto component_offset = info.offset.add(location.index.toInt() * info.component_size);
        auto component_ptr = location.chunk->dataPointerWithOffset(component_offset);
        info.constructor(component_ptr);
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
        --size_;
    };

    const auto location = entityIndexToInternalLocation(index);
    const auto last_item_index = ArchetypeEntityIndex::make(size_ - 1);
    if (index == last_item_index) {
        call_destructors(location);
        return Entity{};
    }
    // moving last entity to index
    const auto source_location = entityIndexToInternalLocation(ArchetypeEntityIndex::make(size_ - 1));
    for (auto& info : operation_helper_.move) {
        const auto source_component_offset = info.offset.add(source_location.index.toInt() * info.component_size);
        const auto dest_component_offset = info.offset.add(location.index.toInt() * info.component_size);
        auto source_ptr = location.chunk->dataPointerWithOffset(source_component_offset);
        auto dest_ptr = location.chunk->dataPointerWithOffset(dest_component_offset);
        info.move(source_ptr, dest_ptr);
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

void Archetype::reserve(size_t capacity) {
    while (capacity > capacity_) {
        allocateChunk();
    }
}

void Archetype::allocateChunk() {
    // TODO: use memory allocator
    Chunk* chunk = reinterpret_cast<Chunk*>(aligned_alloc(alignof(Entity), sizeof(Chunk)));
//    auto chunk = new Chunk{};
    chunks_.push_back(chunk);
    capacity_ += operation_helper_.capacity;
}

void Archetype::freeChunk(Chunk* chunk) {
    // TODO: use memory allocator
    free(chunk);
}

uint32_t Archetype::worldVersion() const noexcept {
    return world_.version();
}

void Archetype::clear() {
    // TODO: entity manager need to update locations
    const auto for_each_location = [this, size = size_](auto&& f) {
        const auto chunk_last_index = ChunkEntityIndex::make(operation_helper_.capacity);
        auto num_items = size;
        ArchetypeInternalEntityLocation location;
        for (auto chunk : chunks_) {
            location.chunk = chunk;
            for (location.index = ChunkEntityIndex::make(0); location.index < chunk_last_index; ++location.index) {
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
    size_ = 0;
}
