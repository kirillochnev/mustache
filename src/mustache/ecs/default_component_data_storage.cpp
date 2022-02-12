#include "default_component_data_storage.hpp"

#include <mustache/utils/logger.hpp>

#include <mustache/ecs/component_factory.hpp>

using namespace mustache;

namespace {
    constexpr auto kChunkCapacity = ChunkCapacity::make(1024 * 16);
}

DefaultComponentDataStorage::DefaultComponentDataStorage(const ComponentIdMask& mask, MemoryManager& memory_manager):
    BaseComponentDataStorage{},
    memory_manager_{&memory_manager},
    component_getter_info_{memory_manager},
    chunk_capacity_{kChunkCapacity},
    chunks_{memory_manager} {
    if (!mask.isEmpty()) {
        component_getter_info_.reserve(mask.componentsCount());

        auto offset = ComponentOffset::make(0u);
        mask.forEachItem([this, &offset, &mask](ComponentId id) {
            const auto& info = ComponentFactory::componentInfo(id);
            if (offset.toInt() == 0) {
                chunk_align_ = static_cast<uint32_t>(info.align);
            }
            ComponentDataGetter getter;
            getter.offset = offset.alignAs(static_cast<uint32_t>(info.align));
            getter.size = static_cast<uint32_t>(info.size);
            component_getter_info_.push_back(getter);
            offset = getter.offset.add(chunk_capacity_.toInt() * info.size);
        });

        chunk_size_ = offset.alignAs(chunk_align_).toInt();
    }
    Logger{}.info("New ComponentDataStorage has been created, components: %s | chunk capacity: %d",
                  mask.toString().c_str(), chunkCapacity().toInt());
}

void DefaultComponentDataStorage::allocateChunk() {
    auto chunk = static_cast<ChunkPtr>(memory_manager_->allocate(chunk_size_, chunk_align_));
    if (chunk == nullptr) {
        throw std::runtime_error("Can not allocate memory for chunk: " + std::to_string(chunks_.size()));
    }
    chunks_.push_back(chunk);
}

void DefaultComponentDataStorage::freeChunk(ChunkPtr chunk) noexcept {
    memory_manager_->deallocate(chunk);
}

uint32_t DefaultComponentDataStorage::capacity() const noexcept {
    return static_cast<uint32_t>(chunk_capacity_.toInt() * chunks_.size());
}

void DefaultComponentDataStorage::reserve(size_t new_capacity) {
    while (chunk_size_ > 0u && capacity() < new_capacity) {
        allocateChunk();
    }
}

void DefaultComponentDataStorage::clear(bool free_chunks) {
    if (free_chunks) {
        for (auto chunk : chunks_) {
            freeChunk(chunk);
        }
        chunks_.clear();
        chunks_.shrink_to_fit();
    }
    size_ = 0;
}

uint32_t DefaultComponentDataStorage::distToChunkEnd(ComponentStorageIndex global_index) const noexcept {
    const auto diff = [](const auto a, const auto b) noexcept{
        return b > a ? b - a : 0;
    };
    const auto storage_size = size();
    const auto index_in_chunk = (global_index % chunkCapacity());
    const auto elements_in_chunk = chunkCapacity().toInt() - index_in_chunk.toInt();
    const auto elements_in_arch = diff(global_index.toInt(), storage_size);
    return std::min(elements_in_arch, elements_in_chunk);
}


void* DefaultComponentDataStorage::getDataSafe(ComponentIndex component_index,
                                               ComponentStorageIndex index) const noexcept {
    if (component_index.isNull() || index.isNull() ||
        !component_getter_info_.has(component_index) || index.toInt() >= size_) {
        return nullptr;
    }
    const auto chunk_index = index % chunk_capacity_;
    const auto& info = component_getter_info_[component_index];
    const auto offset = info.offset.add(info.size * chunk_index.toInt<size_t>());
    auto chunk = chunks_[index / chunk_capacity_];
    return dataPointerWithOffset(chunk, offset);
}

void* DefaultComponentDataStorage::getDataUnsafe(ComponentIndex component_index,
                                                 ComponentStorageIndex index) const noexcept {
    const auto chunk_index = index % chunk_capacity_;
    const auto& info = component_getter_info_[component_index];
    const auto offset = info.offset.add(info.size * chunk_index.toInt<size_t>());
    auto chunk = chunks_[index / chunk_capacity_];
    return dataPointerWithOffset(chunk, offset);
}
