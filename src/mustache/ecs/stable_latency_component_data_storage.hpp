//
// Created by kirill on 19.04.25.
//

#pragma once

#include <mustache/utils/profiler.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/stable_latency_storage.hpp>

#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_info.hpp>
#include <mustache/ecs/base_component_data_storage.hpp>


namespace mustache {
    class MemoryManager;


    // TODO: сделать всегда 2 буффера! BaseComponentDataStorage::decrSize все ломает
    class StableLatencyComponentDataStorage : public BaseComponentDataStorage {
    public:
        StableLatencyComponentDataStorage(const ComponentIdMask& mask, MemoryManager& memory_manager);

        uint32_t capacity() const noexcept override {
            return capacity_ + (buffers_[1].empty() ? 0 : capacity_);
        }

        void reserve(size_t new_capacity) override;

        void clear(bool free_chunks) override {
            size_ = 0;
            migration_pos_ = 0;
            capacity_ = 0;
            if (free_chunks) {
                buffers_[0].clear();
                buffers_[1].clear();
            }
        }

        void* getDataUnsafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept final  {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            const size_t buffer_index = index.toInt() >= migration_pos_;
            const auto& info = component_infos_[component_index];
            const auto block_capacity = capacity_ * (buffer_index + 1ull);
            const auto component_arr_offset = info.offset * block_capacity;
            const auto component_arr_begin = buffers_[buffer_index].data_ + component_arr_offset;
            return component_arr_begin + info.size * index.toInt();
        }

        void* getDataSafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept override {
            if (index.toInt() < size_ && !component_index.isNull() && component_index.toInt() < component_infos_.size()) {
                return getDataUnsafe(component_index, index);
            }
            return nullptr;
        }
        uint32_t distToChunkEnd(ComponentStorageIndex index) const noexcept override {
            if (index.toInt() < migration_pos_) {
                return migration_pos_ - index.toInt();
            }
            return size_ - index.toInt();
        }

        void emplace(ComponentStorageIndex position) override;


        MUSTACHE_INLINE void incSize() noexcept override {
            if (needGrow()) {
                grow();
            }
            Logger{}.debug("[[EMPLACE AT]] Old size: %d, capacity, migration_point: %d", size_, capacity_, migration_pos_);
            if (isMigrationStage()) {
                migrationSteps(migrationStepsCountWhileInsert());
            } else {
                ++migration_pos_;
            }
            ++size_;
        }

        MUSTACHE_INLINE virtual void decrSize() noexcept override {
            --size_;
            migration_pos_ = std::min(size_, migration_pos_);
        }
    private:
        struct ComponentDataInfo {
            size_t offset; // offset in block with capacity = 1
            size_t size; // sizeof component
            ComponentInfo::MoveFunction move;
            ComponentInfo::Destructor   destructor;
        };
        using Buffer = StableLatencyStorageBuffer<std::byte, uint32_t >;
        using Size = uint32_t;

        [[nodiscard]] bool isMigrationStage() const noexcept {
            return size_ >= capacity_;
        }
        [[nodiscard]] bool needGrow() const noexcept  {
            if (buffers_[1].empty()) {
                return size_ >= capacity_;
            }
            return size_ >= 2 * capacity_;
        }

        [[nodiscard]] static Size migrationStepsCountWhileInsert() noexcept {
            return 1;
        }
        void grow();

        bool migrationSteps(Size count = 0);


//        Size size_ = 0;
        Size migration_pos_ = 0;
        Size capacity_ = 0;
        Buffer buffers_[2];
        ArrayWrapper<ComponentDataInfo, ComponentIndex, true> component_infos_; // ComponentIndex -> {offset, size}
        uint32_t  block_align_ = 0;
        uint32_t block_size_ = 0;
    };
}
