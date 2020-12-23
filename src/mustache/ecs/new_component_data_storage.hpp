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

        [[nodiscard]] MUSTACHE_INLINE uint32_t capacity() const noexcept override {
            return capacity_;
        }

        void reserve(size_t new_capacity) override;
        void clear(bool free_chunks = true) override;

        ComponentStorageIndex pushBack() override;

        void* getDataSafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept override {
            if (index.toInt() >= size_ || !components_.has(component_index)) {
                return nullptr;
            }
            return components_[component_index].get(index);
        }

        void* getDataUnsafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept override {
            return components_[component_index].get(index);
        }

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
            explicit ComponentDataHolder(MemoryManager& manager):
                    memory_manager{&manager},
                    data{manager} {

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
            std::vector<std::byte*, Allocator<std::byte*> > data;
            uint32_t component_size;
            uint32_t component_alignment;
        };
        uint32_t capacity_ = 0;
        ArrayWrapper<ComponentDataHolder, ComponentIndex, true> components_;
        MemoryManager* memory_manager_;
    };
}
