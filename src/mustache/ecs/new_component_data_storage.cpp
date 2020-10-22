#include "new_component_data_storage.hpp"
#include "component_factory.hpp"
#include <mustache/utils/logger.hpp>

using namespace mustache;

NewComponentDataStorage::NewComponentDataStorage(const ComponentIdMask& mask) {
    components_.resize(mask.componentsCount());

    ComponentIndex index = ComponentIndex::make(0);
    mask.forEachItem([&index, this](ComponentId id) {
        const auto &info = ComponentFactory::componentInfo(id);
        auto &component = components_[index];
        component.component_size = info.size;
        component.component_alignment = info.align;
        ++index;
    });

    Logger{}.debug("New ComponentDataStorage has been created, components: %s | chunk capacity: %d",
                   mask.toString().c_str(), chunkCapacity().toInt());
}


void NewComponentDataStorage::incSize() noexcept {
    ++size_;
}

void NewComponentDataStorage::decrSize() noexcept {
    --size_;
}

void NewComponentDataStorage::clear(bool free_chunks) {
    size_ = 0;
    if (free_chunks) {
        components_.clear();
        capacity_ = 0;
    }
}

void NewComponentDataStorage::reserve(size_t new_capacity) {
    if (capacity_ >= new_capacity) {
        return;
    }

    while (capacity_ < new_capacity) {
        allocateBlock();
    }
}

NewComponentDataStorage::ElementView NewComponentDataStorage::getElementView(ComponentStorageIndex index) const noexcept {
    return ElementView {this, index};
}
