#include "new_component_data_storage.hpp"
#include "component_factory.hpp"
#include <mustache/utils/logger.hpp>

using namespace mustache;

NewComponentDataStorage::NewComponentDataStorage(const ComponentMask& mask) {
    components_.resize(mask.componentsCount());

    ComponentIndex index = ComponentIndex::make(0);
    mask.forEachComponent([&index, this](ComponentId id) {
        const auto& info = ComponentFactory::componentInfo(id);
        auto& component = components_[index];
        component.component_size = info.size;
        component.component_alignment = info.align;
        ++index;
    });

    Logger{}.debug("New ComponentDataStorage has been created, components: %s | chunk capacity: %d",
                   mask.toString().c_str(), chunkCapacity().toInt());
}

