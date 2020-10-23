#include "component_data_storage.hpp"

#include <mustache/ecs/component_factory.hpp>
#include <mustache/utils/logger.hpp>

using namespace mustache;

namespace {
    constexpr auto kChunkCapacity = ChunkCapacity::make(1024 * 16);
}

ComponentDataStorage::ComponentDataStorage(const ComponentIdMask& mask):
    chunk_capacity_{kChunkCapacity} {
    component_getter_info_.reserve(mask.componentsCount());

    auto offset = ComponentOffset::make(0u);
    mask.forEachItem([this, &offset, &mask](ComponentId id) {
        const auto& info = ComponentFactory::componentInfo(id);
        if (offset.toInt() == 0) {
            chunk_align_ = info.align;
        }
        ComponentDataGetter getter;
        getter.offset = offset.alignAs(info.align);
        getter.size = info.size;
        component_getter_info_.push_back(getter);
        offset = getter.offset.add(chunk_capacity_.toInt() * info.size);
    });

    chunk_size_ = offset.alignAs(chunk_align_).toInt();

    Logger{}.info("New ComponentDataStorage has been created, components: %s | chunk capacity: %d",
                   mask.toString().c_str(), chunkCapacity().toInt());
}


void ComponentDataStorage::allocateChunk() {
    const auto chunk_size = chunk_size_;

    // TODO: use memory allocator
#ifdef _MSC_BUILD
    ChunkPtr chunk = reinterpret_cast<ChunkPtr>(malloc(chunk_size));
#else
    ChunkPtr chunk = reinterpret_cast<ChunkPtr>(aligned_alloc(chunk_align_, chunk_size));
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
