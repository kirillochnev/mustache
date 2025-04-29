//
// Created by kirill on 19.04.25.
//


#include "stable_latency_component_data_storage.hpp"
#include <mustache/utils/memory_manager.hpp>
#include <mustache/utils/profiler.hpp>
#include <mustache/utils/logger.hpp>
#include "component_factory.hpp"
#include <cassert>
#include <algorithm>

using namespace mustache;

namespace {
    constexpr size_t min_initial_capacity = 1;
}

StableLatencyComponentDataStorage::StableLatencyComponentDataStorage(
        const ComponentIdMask& mask,
        MemoryManager& memory_manager) :
        capacity_(0),
        buffers_{Buffer{memory_manager}, Buffer{memory_manager}} {
    MUSTACHE_PROFILER_BLOCK_LVL_0("StableLatencyComponentDataStorage::ctor");

    size_t offset = 0;
    block_align_ = 0;
    uint32_t component_index = 0;
    for (auto id : mask.items()) {
        const auto& info = ComponentFactory::instance().componentInfo(id);
        if (offset == 0) {
            block_align_ = static_cast<uint32_t>(info.align);
        } else {
            offset = (offset + info.align - 1) & ~(info.align - 1);
        }
        Meta meta {
                {nullptr, nullptr},
                info.size,
                offset,
                info.functions.move_constructor_and_destroy,
                id
        };
        meta_.push_back(meta);
        offset += info.size;
        if (get_meta_.size() <= id.toInt()) {
            get_meta_.resize(id.toInt() + 1);
        }
        get_meta_[id.toInt()] = {meta.base, static_cast<uint32_t>(meta.stride),
                                 ComponentIndex::make(component_index++)};
    }

    block_size_ = (offset + block_align_ - 1ull) & ~(block_align_ - 1ull);

    if (block_size_ > 0) {
        const size_t default_allocation = std::max(
                min_initial_capacity,
                (memory_manager.pageSize() / block_size_)
        );
        capacity_ = static_cast<uint32_t>(default_allocation);
    } else {
        capacity_ = static_cast<uint32_t>(memory_manager.pageSize());
    }
    const size_t initial_bytes = capacity_ * block_size_;
    buffers_[0].resize(initial_bytes, block_align_);
    precomputeBases();
}

void StableLatencyComponentDataStorage::clear(bool free_chunks) {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
    size_ = 0;
    migration_pos_ = 0;
    capacity_ = 0;
    if (free_chunks) {
        buffers_[0].clear();
        buffers_[1].clear();
    }
}

void StableLatencyComponentDataStorage::reserve(size_t new_capacity) {
    if (block_size_ == 0) {
        size_ = static_cast<uint32_t >(new_capacity);
        return;
    }

    while (size_ < new_capacity) {
        incSize();
    }
    precomputeBases();
}

void StableLatencyComponentDataStorage::incSize() noexcept {
    if (needGrow()) grow();
    if (isMigrationStage()) {
        migrationSteps(migrationStepsCount());
    } else {
        ++migration_pos_;
    }
    ++size_;
}

void StableLatencyComponentDataStorage::decrSize() noexcept {
    --size_;
    migration_pos_ = std::min(size_, migration_pos_);
}

bool StableLatencyComponentDataStorage::isMigrationStage() const noexcept {
    return size_ >= capacity_;
}

bool StableLatencyComponentDataStorage::needGrow() const noexcept {
    return buffers_[1].empty() ? (size_ >= capacity_) : (size_ >= 2 * capacity_);
}

uint32_t StableLatencyComponentDataStorage::migrationStepsCount() noexcept {
    return 1;
}

void StableLatencyComponentDataStorage::precomputeBases() noexcept {
    const size_t cap1 = static_cast<size_t>(capacity_);
    const size_t cap2 = buffers_[1].empty() ? cap1 : cap1 * 2;
    uint32_t component_index = 0;
    for (auto& meta : meta_) {
        meta.base[0] = buffers_[0].data_ + meta.offset * cap1;
        meta.base[1] = buffers_[1].empty() ? nullptr : buffers_[1].data_ + meta.offset * cap2;
        get_meta_[meta.id.toInt()] = {meta.base, static_cast<uint32_t>(meta.stride),
                                      ComponentIndex::make(component_index++)};
    }
}

void StableLatencyComponentDataStorage::emplace(ComponentStorageIndex position) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    const auto next = position.next();
    reserve(next.toInt());
}

void StableLatencyComponentDataStorage::grow() {
    if (!buffers_[1].empty()) {
        Buffer::swap(buffers_[0], buffers_[1]);
        capacity_ = capacity_ == 0 ? min_initial_capacity : capacity_ * 2;
    }

    const size_t new_bytes = capacity_ * 2 * block_size_;
    buffers_[1].resize(new_bytes, block_align_);

    migration_pos_ = size_;
    precomputeBases();
}

bool StableLatencyComponentDataStorage::migrationSteps(uint32_t count) {
    if (migration_pos_ == 0 || buffers_[1].empty()) {
        return false;
    }
    count = count == 0 ? migration_pos_ : std::min(count, migration_pos_);
    uint32_t start = migration_pos_ - count;
    for (auto& meta : meta_) {
        std::byte* source = meta.base[0] + meta.stride * start;
        std::byte* dest   = meta.base[1] + meta.stride * start;
        for (uint32_t i = 0; i < count; ++i) {
            meta.move_and_destroy(dest, source);
            dest   += meta.stride;
            source += meta.stride;
        }
    }
    migration_pos_ -= count;
    return migration_pos_ == 0;
}

void StableLatencyComponentDataStorage::Buffer::resize(size_t total_size, size_t alignment) {
    clear();
    const std::size_t aligned_size = (total_size + alignment - 1) & ~(alignment - 1);
    data_ = static_cast<std::byte*>(memory_manager_->allocateSmart(
            aligned_size,
            alignment,
            false,
            false
    ));
}

void StableLatencyComponentDataStorage::Buffer::clear() {
    memory_manager_->deallocateSmart(data_);
    data_ = nullptr;
}
