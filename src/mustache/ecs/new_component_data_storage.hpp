#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/memory_manager.hpp>

#include <array>

namespace mustache {
    class NewComponentDataStorage {
    public:
        class ElementView;

        NewComponentDataStorage(const ComponentIdMask& mask, MemoryManager& memory_manager);

        [[nodiscard]] MUSTACHE_INLINE uint32_t capacity() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE uint32_t size() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE bool isEmpty() const noexcept;

        void reserve(size_t new_capacity);
        void incSize() noexcept;
        void decrSize() noexcept;
        void clear(bool free_chunks = true);

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void* getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept;
        ElementView getElementView(ComponentStorageIndex index) const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE ComponentStorageIndex pushBack();
        [[nodiscard]] MUSTACHE_INLINE ComponentStorageIndex lastItemIndex() const noexcept;

    private:
        MUSTACHE_INLINE void allocateBlock();
        enum : uint32_t {
            kComponentBlockSize = 1024 * 16
        };
        static constexpr ChunkCapacity chunkCapacity() noexcept {
            return ChunkCapacity::make(kComponentBlockSize);
        }
        struct ComponentDataHolder {
            ~ComponentDataHolder() {
                clear();
            }
            void clear() {
                for (auto ptr : data) {
                    memory_manager->deallocate(ptr);
                }
            }
            void allocate() {
                auto* ptr = memory_manager->allocate(component_size * kComponentBlockSize, component_alignment);
                data.push_back(static_cast<std::byte*>(ptr));
            }

            template<FunctionSafety _Safety = FunctionSafety::kDefault>
            void* get(ComponentStorageIndex index) const noexcept {
                if constexpr (isSafe(_Safety)) {
                    if (index.toInt() >= kComponentBlockSize * data.size()) {
                        return nullptr;
                    }
                }
                constexpr auto chunk_capacity = chunkCapacity();
                auto block = data[(index / chunk_capacity).toInt()];
                return block + component_size * (index % chunk_capacity).toInt();
            }
            MemoryManager* memory_manager = nullptr;
            std::vector<std::byte*> data;
            uint32_t component_size;
            uint32_t component_alignment;
        };
        uint32_t size_ = 0;
        uint32_t capacity_ = 0;
        ArrayWrapper<ComponentDataHolder, ComponentIndex> components_;
        MemoryManager* memory_manager_;
    };

    uint32_t NewComponentDataStorage::capacity() const noexcept {
        return capacity_;
    }

    uint32_t NewComponentDataStorage::size() const noexcept {
        return size_;
    }

    bool NewComponentDataStorage::isEmpty() const noexcept {
        return size_ < 1u;
    }
    void NewComponentDataStorage::allocateBlock() {
        for (auto& component : components_) {
            component.allocate();
        }
        capacity_ += kComponentBlockSize;
    }


    template<FunctionSafety _Safety>
    void* NewComponentDataStorage::getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (index.toInt() >= size_ || !components_.has(component_index)) {
                return nullptr;
            }
        }
        return components_[component_index].get(index);
    }

    template<FunctionSafety _Safety>
    ComponentStorageIndex NewComponentDataStorage::pushBack() {
        const auto cur_size = size();
        const auto new_size = cur_size + 1;
        reserve(new_size);
        ComponentStorageIndex index = ComponentStorageIndex::make(cur_size);
        incSize();
        return index;
    }

    ComponentStorageIndex NewComponentDataStorage::lastItemIndex() const noexcept {
        return ComponentStorageIndex::make(size() - 1);
    }

    class NewComponentDataStorage::ElementView {
    public:

        [[nodiscard]] ComponentStorageIndex globalIndex() const noexcept {
            return global_index_;
        }

        [[nodiscard]] uint32_t elementArraySize() const noexcept {
            const auto diff = [](const auto a, const auto b) noexcept{
                return b > a ? b - a : 0;
            };
            const auto global_index = globalIndex();
            const auto storage_size = storage_->size();
            const auto entity_index = globalIndex() % chunkCapacity();
            const auto elements_in_chunk = diff(entity_index.toInt(), chunkCapacity().toInt());
            const auto elements_in_arch = diff(global_index.toInt(), storage_size);
            return std::min(elements_in_arch, elements_in_chunk);
        }

        [[nodiscard]] bool isValid() const noexcept {
            return globalIndex() <= storage_->lastItemIndex();
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        void* getData(ComponentIndex component_index) const noexcept {
            return storage_->getData<_Safety>(component_index, globalIndex());
        }

        ElementView& operator+=(uint32_t count) noexcept {
            global_index_ = ComponentStorageIndex::make(global_index_.toInt() + count);
            return *this;
        }

        ElementView& operator++() noexcept {
            return (*this) += 1;
        }
    private:
        friend NewComponentDataStorage;
        const NewComponentDataStorage* storage_;
        ComponentStorageIndex global_index_;
        constexpr ElementView(const NewComponentDataStorage* storage,
                              ComponentStorageIndex index):
                storage_{storage},
                global_index_{index} {
        }
    };
}
