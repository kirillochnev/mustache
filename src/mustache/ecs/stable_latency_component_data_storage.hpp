#pragma once

#include <mustache/utils/default_settings.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/ecs/base_component_data_storage.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_info.hpp>
#include <cstddef>
#include <array>

namespace mustache {

    class MemoryManager;
    class MUSTACHE_EXPORT StableLatencyComponentDataStorage {
    public:
        StableLatencyComponentDataStorage(const ComponentIdMask& mask, MemoryManager& mmgr);
        ~StableLatencyComponentDataStorage() = default;

        uint32_t capacity() const noexcept {
            return capacity_ + (buffers_[1].empty() ? 0 : capacity_);
        }

        void reserve(size_t new_capacity);
        void clear(bool free_chunks);

        MUSTACHE_INLINE void* getDataUnsafe(ComponentIndex ci, ComponentStorageIndex idx) const noexcept {
            const uint32_t comp = ci.toInt();
            const uint32_t i    = idx.toInt();
            const auto& meta       = meta_[comp];
            return meta.base[i >= migration_pos_] + meta.stride * i;
        }

        MUSTACHE_INLINE void* getDataSafe(ComponentIndex ci, ComponentStorageIndex idx) const noexcept {
            if (idx.toInt() < size_ && !ci.isNull() && ci.toInt() < meta_.size()) {
                return getDataUnsafe(ci, idx);
            }
            return nullptr;
        }
        MUSTACHE_INLINE std::pair<void*, ComponentIndex> getDataSafe(ComponentId id, ComponentStorageIndex idx) const noexcept {
            std::pair<void*, ComponentIndex> result = {nullptr, ComponentIndex{}};
            if (get_meta_.size() > id.toInt()) {
                const uint32_t comp = id.toInt();
                const uint32_t i = idx.toInt();
                const auto& meta = get_meta_[comp];
                result = {meta.base[i >= migration_pos_] + meta.stride * i, meta.index};
            }
            return result;
        }
        MUSTACHE_INLINE std::pair<void*, ComponentIndex> getDataUnsafe(ComponentId id, ComponentStorageIndex idx) const noexcept {
            const uint32_t comp = id.toInt();
            const uint32_t i = idx.toInt();
            const auto& meta = get_meta_[comp];
            return {meta.base[i >= migration_pos_] + meta.stride * i, meta.index};
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void* getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                return getDataSafe(component_index, index);
            } else {
                return getDataUnsafe(component_index, index);
            }
        }

        [[nodiscard]] MUSTACHE_INLINE ComponentStorageIndex lastItemIndex() const noexcept {
            return ComponentStorageIndex::make(size_ - 1);
        }

        [[nodiscard]] MUSTACHE_INLINE uint32_t size() const noexcept {
            return size_;
        }

        [[nodiscard]] MUSTACHE_INLINE bool isEmpty() const noexcept {
            return size_ == 0u;
        }

        [[nodiscard]] MUSTACHE_INLINE uint32_t distToChunkEnd(ComponentStorageIndex index) const noexcept {
            if (index.toInt() < migration_pos_) {
                return migration_pos_ - index.toInt();
            }
            return size_ - index.toInt();
        }

        void emplace(ComponentStorageIndex pos);
        void incSize() noexcept;
        void decrSize() noexcept;

    private:
        struct GetMeta {
            std::array<std::byte*, 2> base;
            uint32_t   stride;
            ComponentIndex index;
        };

        struct Meta {
            std::array<std::byte*, 2> base;
            size_t    stride;
            size_t offset;
            ComponentInfo::MoveFunction move_and_destroy;
            ComponentId id;
        };
        struct Buffer {
            std::byte* data_ = nullptr;
            MemoryManager* memory_manager_ = nullptr;
            explicit Buffer(MemoryManager& manager):
                    memory_manager_{&manager} {
            }
            ~Buffer() {
                clear();
            }
            const std::byte* data() const noexcept {
                return data_;
            }
            std::byte* data() noexcept {
                return data_;
            }
            bool empty() const noexcept {
                return data_ == nullptr;
            }
            void resize(size_t total_size, size_t alignment);
            void clear();
            static void swap(Buffer& a, Buffer& b) noexcept {
                std::swap(a.data_, b.data_);
            }
        };

        void precomputeBases() noexcept;
        [[nodiscard]] bool isMigrationStage() const noexcept;
        [[nodiscard]] bool needGrow() const noexcept;
        [[nodiscard]] static uint32_t migrationStepsCount() noexcept;
        void grow();
        bool migrationSteps(uint32_t count = 0);
        uint32_t size_ = 0;
        vector<GetMeta> get_meta_;
        uint32_t migration_pos_ = 0;
        uint32_t capacity_ = 0;
        std::array<Buffer, 2> buffers_;
        uint32_t block_align_ = 0;
        size_t block_size_  = 0;
        vector<Meta> meta_;
    };
}
