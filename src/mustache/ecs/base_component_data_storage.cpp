#include "base_component_data_storage.hpp"

using namespace mustache;

void BaseComponentDataStorage::emplace(ComponentStorageIndex position) {
    const auto next = position.next();
    reserve(next.toInt());
    if (size_ < next.toInt()) {
        size_ = next.toInt();
    }
}
