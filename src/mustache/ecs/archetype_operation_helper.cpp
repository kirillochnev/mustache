#include "archetype_operation_helper.hpp"
#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_factory.hpp>

using namespace mustache;
namespace {
    ComponentOffset alignComponentOffset(ComponentOffset prev, size_t align) noexcept {
        // TODO: use std::align
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
        num_components{mask.componentsCount()},
        entity_offset{entityOffset(num_components)},
        version_offset{versionOffset()} {

    ArrayWrapper<std::vector<ComponentId>, ComponentIndex> component_index_to_component_id{mask.components()};

    ArrayWrapper<std::vector<ComponentOffset>, ComponentIndex> offsets;
    uint32_t element_size {static_cast<uint32_t>(sizeof(Entity))};
    for(auto id : component_index_to_component_id) {
        const auto& info = ComponentFactory::componentInfo(id);
        element_size = alignComponentOffset(ComponentOffset::make(element_size), info.align).toInt();
        offsets.push_back(ComponentOffset::make(element_size));
        element_size += static_cast<uint32_t>(info.size);
    }
    const auto capacity = (Chunk::kChunkSize - entity_offset.toInt()) / element_size;
    index_of_last_entity_in_chunk = ChunkEntityIndex::make(capacity - 1);
    for(auto& offset : offsets) {
        offset = ComponentOffset::make(offset.toInt() * capacity + entity_offset.toInt());
    }

    ComponentIndex component_index = ComponentIndex::make(0);

    for (auto component_id : component_index_to_component_id) {
        if (!component_id_to_component_index.has(component_id)) {
            component_id_to_component_index.resize(component_id.next().toInt());
        }
        component_id_to_component_index[component_id] = component_index;
        const auto& info = ComponentFactory::componentInfo(component_id);

        if (info.functions.create) {
            insert.push_back(InsertInfo {
                    info.functions.create,
                    component_index
            });
        }
        if (info.functions.destroy) {
            destroy.push_back(DestroyInfo {
                    info.functions.destroy,
                    component_index
            });
        }
        if (info.functions.move) {
            internal_move.push_back(InternalMoveInfo {
                    info.functions.move
            });
        }
        ExternalMoveInfo external_move_info;
        external_move_info.constructor = info.functions.create;
        external_move_info.move = info.functions.move_constructor;
        external_move_info.id = component_id;
        external_move.push_back(external_move_info);

        ++component_index;
    }
}
