#pragma once

#include <mustache/ecs/component_mask.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/memory_manager.hpp>

#include <array>

namespace mustache {
    using ChunkHandler = uintptr_t;
    class ComponentDataStorage {
    public:
        class ElementView;

        ComponentDataStorage(const ComponentIdMask& mask, MemoryManager& memory_manager);

        [[nodiscard]] MUSTACHE_INLINE uint32_t capacity() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE uint32_t size() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE bool isEmpty() const noexcept;

        MUSTACHE_INLINE void reserve(size_t new_capacity);
        MUSTACHE_INLINE void incSize() noexcept;
        MUSTACHE_INLINE void decrSize() noexcept;
        MUSTACHE_INLINE void clear(bool free_chunks = true);

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void* getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept;
        MUSTACHE_INLINE ElementView getElementView(ComponentStorageIndex index) const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE ComponentStorageIndex pushBack();
        [[nodiscard]] MUSTACHE_INLINE ComponentStorageIndex lastItemIndex() const noexcept;

    private:
        struct ComponentDataGetter {
            ComponentOffset offset;
            uint32_t size;
        };

        using ChunkPtr = std::byte*;

        template <typename T = std::byte>
        [[nodiscard]] static T* data(ChunkPtr chunk) noexcept {
            return reinterpret_cast<T*>(chunk);
        }

        template <typename T = void>
        [[nodiscard]] static T* dataPointerWithOffset(ChunkPtr chunk, ComponentOffset offset) noexcept {
            return reinterpret_cast<T*>(offset.apply(data(chunk)));
        }

        [[nodiscard]] MUSTACHE_INLINE uint32_t componentsCount() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE ChunkCapacity chunkCapacity() const noexcept;
        MUSTACHE_INLINE void reserveForNextItem();

        void allocateChunk();
        void freeChunk(ChunkPtr chunk) noexcept;

        MemoryManager* memory_manager_;
        ArrayWrapper<ComponentDataGetter, ComponentIndex, true> component_getter_info_; // ComponentIndex -> {offset, size}
        uint32_t size_{0u};
        ChunkCapacity chunk_capacity_;
        ArrayWrapper<ChunkPtr, ChunkIndex, true> chunks_;
        uint32_t chunk_size_ {0u};
        uint32_t chunk_align_ {0u};
    };

    uint32_t ComponentDataStorage::capacity() const noexcept {
        return chunk_capacity_.toInt() * chunks_.size();
    }

    uint32_t ComponentDataStorage::size() const noexcept {
        return size_;
    }

    bool ComponentDataStorage::isEmpty() const noexcept {
        return size_ == 0u;
    }

    void ComponentDataStorage::reserve(size_t new_capacity) {
        while (capacity() < new_capacity) {
            allocateChunk();
        }
    }

    void ComponentDataStorage::incSize() noexcept {
        ++size_;
    }

    void ComponentDataStorage::decrSize() noexcept {
        --size_;
    }

    template<FunctionSafety _Safety>
    void* ComponentDataStorage::getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (component_index.isNull() || index.isNull() ||
                !component_getter_info_.has(component_index) || index.toInt() >= size_) {
                return nullptr;
            }
        }
        const auto chunk_index = index % chunk_capacity_;
        const auto& info = component_getter_info_[component_index];
        const auto offset = info.offset.add(info.size * chunk_index.toInt());
        auto chunk = chunks_[index / chunk_capacity_];
        return dataPointerWithOffset(chunk, offset);
    }

    void ComponentDataStorage::clear(bool free_chunks) {
        if (free_chunks) {
            for (auto chunk : chunks_) {
                freeChunk(chunk);
            }
            chunks_.clear();
        }
        size_ = 0;
    }

    void ComponentDataStorage::reserveForNextItem() {
        reserve(size_ + 1);
    }

    uint32_t ComponentDataStorage::componentsCount() const noexcept {
        return component_getter_info_.size();
    }

    ChunkCapacity ComponentDataStorage::chunkCapacity() const noexcept {
        return chunk_capacity_;
    }

    template<FunctionSafety _Safety>
    ComponentStorageIndex ComponentDataStorage::pushBack() {
        reserveForNextItem();
        ComponentStorageIndex index = ComponentStorageIndex::make(size_);
        incSize();
        return index;
    }

    ComponentStorageIndex ComponentDataStorage::lastItemIndex() const noexcept {
        return ComponentStorageIndex::make(size_ - 1);
    }

    class ComponentDataStorage::ElementView { // TODO: rename
    public:

        [[nodiscard]] ComponentStorageIndex globalIndex() const noexcept {
            return global_index_;
        }

        [[nodiscard]] uint32_t elementArraySize() const noexcept { // TODO: create class to access arrays
            const auto diff = [](const auto a, const auto b) noexcept{
                return b > a ? b - a : 0;
            };
            const auto global_index = globalIndex();
            const auto storage_size = storage_->size();
            const auto index_in_chunk = (global_index_ % storage_->chunkCapacity());
            const auto elements_in_chunk = storage_->chunkCapacity().toInt() - index_in_chunk.toInt();
            const auto elements_in_arch = diff(global_index.toInt(), storage_size);
            return std::min(elements_in_arch, elements_in_chunk);
        }

        [[nodiscard]] bool isValid() const noexcept {
            return globalIndex() <= storage_->lastItemIndex();
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        void* getData(ComponentIndex component_index) const noexcept {
            return storage_->getData<_Safety>(component_index, global_index_);
        }

        ElementView& operator+=(uint32_t count) noexcept {
            global_index_ = ComponentStorageIndex::make(global_index_.toInt() + count);
            return *this;
        }

        ElementView& operator++() noexcept {
            return (*this) += 1;
        }

    protected:
        [[nodiscard]] static ComponentStorageIndex globalIndex(const ComponentDataStorage* storage,
                ChunkIndex chunk_index, ChunkItemIndex item_index) noexcept {
            return ComponentStorageIndex::make(
                    chunk_index.toInt() * storage->chunkCapacity().toInt() + item_index.toInt()
            );
        }

        friend ComponentDataStorage;
        const ComponentDataStorage* storage_;
        ComponentStorageIndex global_index_; // TODO: rename?
        ElementView(const ComponentDataStorage* storage, ComponentStorageIndex index):
                storage_{storage},
                global_index_{index} {

        }
    };

    ComponentDataStorage::ElementView ComponentDataStorage::getElementView(ComponentStorageIndex index) const noexcept {
        return ElementView{this, index };
    }
}
