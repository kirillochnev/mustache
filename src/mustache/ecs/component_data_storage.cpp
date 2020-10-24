#include "component_data_storage.hpp"

#include <mustache/ecs/component_factory.hpp>
#include <mustache/utils/logger.hpp>

using namespace mustache;

namespace {
    constexpr auto kChunkCapacity = ChunkCapacity::make(1024 * 16);
}

ComponentDataStorage::ComponentDataStorage(const ComponentIdMask& mask, MemoryManager& memory_manager):
    memory_manager_{&memory_manager},
    component_getter_info_{memory_manager},
    chunk_capacity_{kChunkCapacity},
    chunks_{memory_manager} {
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
    auto chunk = static_cast<ChunkPtr>(memory_manager_->allocate(chunk_size_, chunk_align_));
    if (chunk == nullptr) {
        throw std::runtime_error("Can not allocate memory for chunk: " + std::to_string(chunks_.size()));
    }
    chunks_.push_back(chunk);
}

void ComponentDataStorage::freeChunk(ChunkPtr chunk) noexcept {
    memory_manager_->deallocate(chunk);
}
