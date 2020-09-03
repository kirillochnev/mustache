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

ArchetypeEntityIndex Archetype::insert(Entity entity) {
    reserve(size() + 1);
    const auto location = entityIndexToInternalLocation(ArchetypeEntityIndex::make(size_));
    updateVersion(world_.version(), operation_helper_.num_components,
            operation_helper_.version_offset, location.chunk);
    const auto entity_offset = operation_helper_.entity_offset.add(location.index.toInt() * sizeof(Entity));
    new (location.chunk->dataPointerWithOffset<Entity>(entity_offset)) Entity (entity);
    for (const auto& info : operation_helper_.insert) {
        const auto component_offset = info.offset.add(location.index.toInt() * info.component_size);
        auto component_ptr = location.chunk->dataPointerWithOffset(component_offset);
        info.constructor(component_ptr);
    }
    return ArchetypeEntityIndex::make(size_++);
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

uint32_t Archetype::worldVersion() const noexcept {
    return world_.version();
}

void Archetype::clear() {
    size_ = 0;
    // TODO: add destructor call
}
