// stable_latency_component_data_storage_optimized.hpp
#pragma once

#include <mustache/utils/stable_latency_storage.hpp>
#include <mustache/ecs/base_component_data_storage.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_info.hpp>
#include <cstddef>
#include <array>

namespace mustache {

    class MemoryManager;

/// Оптимизированное хранилище с предвычисленными метаданными
    class StableLatencyComponentDataStorage final : public BaseComponentDataStorage {
    public:
        StableLatencyComponentDataStorage(const ComponentIdMask& mask, MemoryManager& mmgr);
        ~StableLatencyComponentDataStorage() = default;

        uint32_t capacity() const noexcept override {
            return capacity_ + (buffers_[1].empty() ? 0 : capacity_);
        }

        void reserve(size_t new_capacity) override;
        void clear(bool free_chunks) override;

        MUSTACHE_INLINE void* getDataUnsafe(ComponentIndex ci, ComponentStorageIndex idx) const noexcept final {
            const uint32_t comp = ci.toInt();
            const uint32_t i    = idx.toInt();
            const uint32_t useHigh  = (i >= migration_pos_);
            const auto& meta       = meta_[comp];
//            std::byte* base     = useHigh ? meta.base[1] : meta.base[0];
            return meta.base[i >= migration_pos_] + meta.stride * i;
        }

        MUSTACHE_INLINE void* getDataSafe(ComponentIndex ci, ComponentStorageIndex idx) const noexcept override {
            if (idx.toInt() < size_ && !ci.isNull() && ci.toInt() < meta_.size()) {
                return getDataUnsafe(ci, idx);
            }
            return nullptr;
        }

        uint32_t distToChunkEnd(ComponentStorageIndex index) const noexcept override {
            if (index.toInt() < migration_pos_) {
                return migration_pos_ - index.toInt();
            }
            return size_ - index.toInt();
        }

        void emplace(ComponentStorageIndex pos) override;
        void incSize() noexcept override;
        void decrSize() noexcept override;

    private:
        struct Meta {
            std::array<std::byte*, 2> base;
            size_t    stride;
            size_t offset;
            ComponentInfo::MoveFunction move_and_destroy;
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

        uint32_t migration_pos_ = 0;
        uint32_t capacity_;
        std::vector<Meta> meta_;
        std::array<Buffer, 2> buffers_;
        uint32_t block_align_ = 0;
        size_t block_size_  = 0;
    };



} // namespace mustache
