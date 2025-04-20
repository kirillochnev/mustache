//
// Created by kirill on 19.04.25.
//

#include "stable_latency_component_data_storage.hpp"
#include "mustache/utils/logger.hpp"
#include "component_factory.hpp"
#include <cassert>

using namespace mustache;
namespace {
    constexpr size_t initial_capacity = 16 * 1024;
}

StableLatencyComponentDataStorage::StableLatencyComponentDataStorage(const ComponentIdMask& mask, MemoryManager&):
    capacity_{initial_capacity} {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
    if (!mask.isEmpty()) {
        component_infos_.reserve(mask.componentsCount());

        auto offset = ComponentOffset::make(0u);
        mask.forEachItem([this, &offset](ComponentId id) {
            const auto& info = ComponentFactory::instance().componentInfo(id);
            if (offset.toInt() == 0) {
                block_align_ = static_cast<uint32_t>(info.align);
            } else {
                offset = offset.alignAs(static_cast<uint32_t>(info.align));
            }
            ComponentDataInfo data_info;
            data_info.offset = offset.toInt();
            data_info.size = info.size;
            data_info.move = info.functions.move_constructor;
            data_info.destructor = info.functions.destroy;
            component_infos_.push_back(data_info);
            offset = offset.add(info.size);
        });

        block_size_ = offset.alignAs(block_align_).toInt();
    }
    buffers_[0].resize(capacity_ * block_size_, block_align_);
//    Logger{}.debug("StableLatencyComponentDataStorage has been created, components: %s | initial capacity: %d",
//                   mask.toString().c_str(), capacity_);
}

void StableLatencyComponentDataStorage::grow() {
    MUSTACHE_PROFILER_BLOCK_LVL_1(__FUNCTION__);
    if (!buffers_[1].empty()) {
        Buffer::swap(buffers_[0], buffers_[1]);
        capacity_ = (capacity_ == 0ull) ? initial_capacity : (capacity_ * 2ull);
    }

    buffers_[1].resize( capacity_ * 2 * block_size_, block_align_);
    migration_pos_ = size_;
}

void StableLatencyComponentDataStorage::reserve(size_t new_capacity) {
    if (block_size_ == 0) {
        size_ = static_cast<uint32_t >(new_capacity);
        return;
    }
//    Logger{}.debug("[[INSERT]] new size %d", new_capacity);
//    assert(new_capacity == size_ + 1);
    while (size_ < new_capacity) {
        incSize();
    }
}

void StableLatencyComponentDataStorage::emplace(ComponentStorageIndex position) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    const auto next = position.next();
    reserve(next.toInt());
}

bool StableLatencyComponentDataStorage::migrationSteps(StableLatencyComponentDataStorage::Size count) {
    if (migration_pos_ == 0 || buffers_[1].empty()) {
        return false;
    }
    count = count == 0 ? migration_pos_ : std::min(count, migration_pos_);
    const auto migration_start = migration_pos_ - count;
//    Logger{}.debug("\t[[MIGRATION]] steps: %d, migration_pos_: %d, migration_start: %d", count, migration_pos_, migration_start);
    for (const auto& info : component_infos_) {
        const auto component_arr_offset_source = info.offset * capacity_;
        const auto component_arr_offset_dest = info.offset * capacity_ * 2;
        const auto component_arr_begin_source = buffers_[0].data_ + component_arr_offset_source;
        const auto component_arr_begin_dest = buffers_[1].data_ + component_arr_offset_dest;

        auto source = component_arr_begin_source + info.size * migration_start;
        auto dest = component_arr_begin_dest + info.size * migration_start;
//                auto source = buffers_[0].data_ + (info.offset * capacity_ + migration_start) * info.size;
//                auto dest = buffers_[1].data_ + (2 * info.offset * capacity_ + migration_start) * info.size;
        for (Size i = 0; i < count; ++i) {
            info.move(dest, source);
            if (info.destructor) {
                info.destructor(source);
            }
            dest += info.size;
            source += info.size;
        }
    }
    migration_pos_ -= count;
    return migration_pos_ == 0;
}
