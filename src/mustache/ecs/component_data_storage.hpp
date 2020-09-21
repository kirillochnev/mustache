#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/chunk.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/utils/array_wrapper.hpp>

namespace mustache {
    class ComponentDataStorage {
    public:
        explicit ComponentDataStorage(const ComponentMask& mask);

        [[nodiscard]] uint32_t capacity() const noexcept {
            return chunk_capacity_.toInt() * chunks_.size();
        }

        [[nodiscard]] uint32_t size() const noexcept {
            return size_;
        }

        [[nodiscard]] bool isEmpty() const noexcept {
            return size_ == 0u;
        }

        void reserve(size_t new_capacity) {
            while (capacity() < new_capacity) {
                allocateChunk();
            }
        }

        void incSize() noexcept {
            ++size_;
        }

        void decrSize() noexcept {
            --size_;
        }

        DataLocation locationOf(ComponentId id) const noexcept {
            (void ) id;
            /// TODO: impl me
            return DataLocation::null();
        }
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getEntityData(ComponentStorageIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (index.isNull() || index.toInt() >= size_) {
                    return nullptr;
                }
            }
            const auto index_at_chunk = index % chunk_capacity_;
            const auto read_offset = entityOffset().add(sizeof(Entity) * index_at_chunk.toInt());
            return chunks_[index / chunk_capacity_]->dataPointerWithOffset(read_offset);
        }
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getData(DataLocation location, ComponentStorageIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (location.isNull() || index.isNull() ||
                    !component_getter_info_.has(location) || index.toInt() >= size_) {
                    return nullptr;
                }
            }
            const auto& getter_info = component_getter_info_[location];
            return getter_info.getData(*chunks_[index / chunk_capacity_], index % chunk_capacity_);
        }

        void clear(bool free_chunks = true) {
            if (free_chunks) {
                for (auto chunk : chunks_) {
                    freeChunk(chunk);
                }
                chunks_.clear();
            }
            size_ = 0;
        }

        void reserveForNextItem() {
            reserve(size_ + 1);
        }

        [[nodiscard]] uint32_t componentsCount() const noexcept {
            return component_getter_info_.size();
        }

        [[nodiscard]] ComponentOffset entityOffset() const noexcept {
            return ComponentOffset::makeAligned(ComponentOffset::make(sizeof(WorldId) * componentsCount()),
                    alignof(Entity));
        }
    private:

        struct ComponentDataGetter {
            [[nodiscard]] void* getData(Chunk& chunk, ChunkEntityIndex index) const noexcept {
                return chunk.dataPointerWithOffset(offset.add(index.toInt() * size));
            }
            ComponentOffset offset;
            uint32_t size;
        };

        friend class Archetype;
        void allocateChunk();
        void freeChunk(Chunk* chunk) noexcept;

        uint32_t size_{0u};
        ChunkCapacity chunk_capacity_;
        ArrayWrapper<std::vector<ComponentDataGetter>, DataLocation> component_getter_info_; // DataLocation -> {offset, size}
        ArrayWrapper<std::vector<Chunk*>, ChunkIndex> chunks_;
    };
}
