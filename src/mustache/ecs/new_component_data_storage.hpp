#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/memory_manager.hpp>
#include <mustache/ecs/base_component_data_storage.hpp>

#include <array>

namespace mustache {
    class NewComponentDataStorage : public BaseComponentDataStorage {
    public:
        class ElementView;

        NewComponentDataStorage(const ComponentIdMask& mask, MemoryManager& memory_manager);
        ~NewComponentDataStorage();

        [[nodiscard]] MUSTACHE_INLINE uint32_t capacity() const noexcept override {
            return capacity_;
        }

        void reserve(size_t new_capacity) override;
        void clear(bool free_chunks = true) override;

        void* getDataSafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept override;

        void* getDataUnsafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept override;

        uint32_t distToChunkEnd(ComponentStorageIndex global_index) const noexcept override {
            const auto diff = [](const auto a, const auto b) noexcept{
                return b > a ? b - a : 0;
            };
            const auto storage_size = size();
            const auto entity_index = global_index % chunkCapacity();
            const auto elements_in_chunk = diff(entity_index.toInt(), chunkCapacity().toInt());
            const auto elements_in_arch = diff(global_index.toInt(), storage_size);
            return std::min(elements_in_arch, elements_in_chunk);
        }

        static ChunkCapacity chunkCapacity() noexcept;
    private:
        MUSTACHE_INLINE void allocateBlock();
        struct ComponentDataHolder;
        uint32_t capacity_ = 0;
        ArrayWrapper<ComponentDataHolder, ComponentIndex, true> components_;
        MemoryManager* memory_manager_;
    };
}
