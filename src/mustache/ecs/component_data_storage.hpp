#pragma once

#include <mustache/ecs/chunk.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/utils/array_wrapper.hpp>

namespace mustache {
    class ComponentDataStorage {
    public:
        [[nodiscard]] uint32_t capacity() const noexcept {
            return chunk_capacity_ * chunks_.size();
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

        void* getData(DataLocation location, ComponentStorageIndex index) const noexcept {
            (void ) location;
            (void ) index;
            /// TODO: impl me
            return nullptr;
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
    private:
        friend class Archetype;
        void allocateChunk();
        void freeChunk(Chunk* chunk) noexcept;

        uint32_t size_{0u};
        uint32_t chunk_capacity_{0u};
        ComponentMask mask_;
        ArrayWrapper<std::vector<Chunk*>, ChunkIndex> chunks_;
    };
}
