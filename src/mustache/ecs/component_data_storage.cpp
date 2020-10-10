#include "component_data_storage.hpp"

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/utils/logger.hpp>

using namespace mustache;
namespace {
    constexpr uint32_t getChunkAlign(uint32_t num_of_components) noexcept {
        const auto is_world_id_align_ok = (num_of_components * sizeof(WorldVersion)
                + alignof(WorldVersion)) % alignof(Entity) == 0;
        return is_world_id_align_ok ? alignof(WorldVersion) : alignof(Entity);
    }
}

ComponentDataStorage::ComponentDataStorage(const ComponentIdMask& mask):
    chunk_capacity_{calculateChunkCapacity(mask)},
    element_size_{calculateElementSize(mask)} {
    component_getter_info_.reserve(mask.componentsCount());

    const auto entity_offset = entityOffset();
    auto offset = entity_offset.add(sizeof(Entity) * chunk_capacity_.toInt());
    mask.forEachItem([this, &offset, entity_offset, &mask](ComponentId id) {
        const auto &info = ComponentFactory::componentInfo(id);
        ComponentDataGetter getter;
        getter.offset = offset.alignAs(info.align);
        getter.size = info.size;
        component_getter_info_.push_back(getter);
        offset = getter.offset.add(chunk_capacity_.toInt() * info.size);
    });

    Logger{}.info("New ComponentDataStorage has been created, components: %s | chunk capacity: %d",
                   mask.toString().c_str(), chunkCapacity().toInt());
}

uint32_t ComponentDataStorage::calculateElementSize(const ComponentIdMask& mask) const noexcept {
    // TODO: fix me! element size is too big
    uint32_t element_size = sizeof(Entity);
    mask.forEachItem([&element_size](ComponentId id) {
        const auto& info = ComponentFactory::componentInfo(id);
        const auto offset = ComponentOffset::makeAligned(ComponentOffset::make(element_size), info.align);
        element_size = offset.add(info.size).toInt();
    });
    return element_size;
}

ChunkCapacity ComponentDataStorage::calculateChunkCapacity(const ComponentIdMask& mask) const noexcept {
    if constexpr (false) {
        constexpr uint32_t kChunkSize = 1024 * 1024;
        const auto unused_space_size = (kChunkSize - sizeof(WorldVersion) * mask.componentsCount());
        return ChunkCapacity::make(unused_space_size / calculateElementSize(mask));
    } else {
        return ChunkCapacity::make(1024 * 16);
    }
}

void ComponentDataStorage::allocateChunk() {
    const auto chunk_alignment = getChunkAlign(componentsCount());
    const auto chunk_size = ComponentOffset::make(element_size_ * chunk_capacity_.toInt()).alignAs(chunk_alignment).toInt();

    // TODO: use memory allocator
#ifdef _MSC_BUILD
    (void) chunk_alignment;
    ChunkPtr chunk = reinterpret_cast<ChunkPtr>(malloc(chunk_size));
#else
    ChunkPtr chunk = reinterpret_cast<ChunkPtr>(aligned_alloc(chunk_alignment, chunk_size));
#endif
    chunks_.push_back(chunk);
}

void ComponentDataStorage::freeChunk(ChunkPtr chunk) noexcept {
    // TODO: use memory allocator
#ifdef _MSC_BUILD
    free (chunk);
#else
    free(chunk);
#endif
}
