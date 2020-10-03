#include "component_data_storage.hpp"
#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <memory>
#include <mustache/utils/logger.hpp>

using namespace mustache;
namespace {
    constexpr uint32_t getChunkAlign(uint32_t num_of_components) noexcept {
        const auto is_world_id_align_ok = (num_of_components * sizeof(WorldVersion)
                + alignof(WorldVersion)) % alignof(Entity) == 0;
        return is_world_id_align_ok ? alignof(WorldVersion) : alignof(Entity);
    }
}

ComponentDataStorage::ComponentDataStorage(const ComponentIdMask& mask) {
    component_getter_info_.reserve(mask.componentsCount());

    element_size_ = sizeof(Entity);

    mask.forEachItem([this](ComponentId id) {
        const auto &info = ComponentFactory::componentInfo(id);
        ComponentDataGetter getter;
        getter.offset = ComponentOffset::makeAligned(ComponentOffset::make(element_size_), info.align);
        getter.size = info.size;
        element_size_ = getter.offset.add(info.size).toInt();
        component_getter_info_.push_back(getter);
    });

    const auto entity_offset = entityOffset();

    chunk_capacity_ = ChunkCapacity::make(1024 * 16);//ChunkCapacity::make((Chunk::kChunkSize - entity_offset.toInt()) / element_size_);

    for (auto& getter : component_getter_info_) {
        getter.offset = entity_offset.add(getter.offset.toInt() * chunk_capacity_.toInt());
    }

    Logger{}.info("New ComponentDataStorage has been created, components: %s | chunk capacity: %d",
                   mask.toString().c_str(), chunkCapacity().toInt());
}

void ComponentDataStorage::allocateChunk() {
    const auto chunk_size = element_size_ * chunk_capacity_.toInt();
    // TODO: use memory allocator
#ifdef _MSC_BUILD
    ChunkPtr chunk = reinterpret_cast<ChunkPtr>(malloc(chunk_size));
#else
    const auto chunk_alignment = getChunkAlign(componentsCount());
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
