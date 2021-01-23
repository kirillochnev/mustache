#pragma once

#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/base_component_data_storage.hpp>

#include <mustache/utils/array_wrapper.hpp>

namespace mustache {
    class MemoryManager;

    class DefaultComponentDataStorage : public BaseComponentDataStorage {
    public:
        DefaultComponentDataStorage(const ComponentIdMask& mask, MemoryManager& memory_manager);

        uint32_t capacity() const noexcept override;

        void reserve(size_t new_capacity) override;

        void clear(bool free_chunks) override;

        uint32_t distToChunkEnd(ComponentStorageIndex index) const noexcept override;

        [[nodiscard]] MUSTACHE_INLINE ChunkCapacity chunkCapacity() const noexcept {
            return chunk_capacity_;
        }

        void* getDataSafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept override;

        void* getDataUnsafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept override;

    protected:
        struct ComponentDataGetter {
            ComponentOffset offset;
            uint32_t size;
        };

        using ChunkPtr = std::byte*;

        void allocateChunk();
        void freeChunk(ChunkPtr chunk) noexcept;

        template <typename T = std::byte>
        [[nodiscard]] MUSTACHE_INLINE static T* data(ChunkPtr chunk) noexcept {
            return reinterpret_cast<T*>(chunk);
        }

        template <typename T = void>
        [[nodiscard]] MUSTACHE_INLINE static T* dataPointerWithOffset(ChunkPtr chunk, ComponentOffset offset) noexcept {
            return reinterpret_cast<T*>(offset.apply(data(chunk)));
        }

        MemoryManager* memory_manager_ = nullptr;
        ArrayWrapper<ComponentDataGetter, ComponentIndex, true> component_getter_info_; // ComponentIndex -> {offset, size}
        ChunkCapacity chunk_capacity_;
        ArrayWrapper<ChunkPtr, ChunkIndex, true> chunks_;
        uint32_t chunk_size_ {0u};
        uint32_t chunk_align_ {0u};
    };
}
