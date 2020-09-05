#include "archetype_operation_helper.hpp"
#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_factory.hpp>

using namespace mustache;
namespace {
    ComponentOffset alignComponentOffset(ComponentOffset prev, size_t align) noexcept {
        const auto space = prev.toInt() % align;
        return prev.add(space > 0 ? align - space : 0);
    }

    ComponentOffset versionOffset() noexcept {
        return ComponentOffset::make(0);
    }
    ComponentOffset entityOffset(const size_t compoments_count) noexcept {
        const auto not_aligned_offset = versionOffset().add(sizeof(uint32_t) * compoments_count);
        return alignComponentOffset(not_aligned_offset, alignof(Entity));
    }
}

ArchetypeOperationHelper::ArchetypeOperationHelper(const ComponentMask& mask):
        component_index_to_component_id{mask.components()},
        num_components{static_cast<uint32_t>(component_index_to_component_id.size())},
        entity_offset{entityOffset(num_components)},
        version_offset{versionOffset()} {

    std::vector<ComponentOffset> offsets;
    size_t element_size {sizeof(Entity)};
    for(auto id : component_index_to_component_id) {
        const auto& info = ComponentFactory::componentInfo(id);
        element_size = alignComponentOffset(ComponentOffset::make(element_size), info.align).toInt();
        offsets.push_back(ComponentOffset::make(element_size));
        element_size += info.size;
    }
    capacity = (Chunk::kChunkSize - entity_offset.toInt()) / element_size;
    for(auto& offset : offsets) {
        offset = ComponentOffset::make(offset.toInt() * capacity + entity_offset.toInt());
    }

    ComponentIndex component_index = ComponentIndex::make(0);

    for (auto component_id : component_index_to_component_id) {
        if (component_id_to_component_index.size() <= component_id.toInt()) {
            component_id_to_component_index.resize(component_id.toInt() + 1);
        }
        component_id_to_component_index[component_id.toInt()] = component_index;
        const auto& info = ComponentFactory::componentInfo(component_id);
        get.push_back(GetComponentInfo{offsets[component_index.toInt()], static_cast<uint32_t>(info.size)});
        if (info.functions.create) {
            insert.push_back(InsertInfo {
                    offsets[component_index.toInt()],
                    static_cast<uint32_t>(info.size),
                    info.functions.create
            });
        }
        if (info.functions.destroy) {
            destroy.push_back(DestroyInfo {
                    offsets[component_index.toInt()],
                    static_cast<uint32_t>(info.size),
                    info.functions.destroy
            });
        }
        if (info.functions.move) {
            move.push_back(MoveInfo {
                    offsets[component_index.toInt()],
                    static_cast<uint32_t>(info.size),
                    info.functions.move
            });
        }
        ++component_index;
    }
}
