#include "base_component_data_storage.hpp"

#include <mustache/utils/profiler.hpp>

using namespace mustache;

void BaseComponentDataStorage::emplace(ComponentStorageIndex position) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    const auto next = position.next();
    reserve(next.toInt());
    if (size_ < next.toInt()) {
        size_ = next.toInt();
    }
}
