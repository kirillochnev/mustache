#pragma once

#include <mustache/utils/type_info.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/default_settings.hpp>
#include <mustache/ecs/id_deff.hpp>

#include <vector>

namespace mustache {
    struct ComponentIdMask;

    class ArchetypeOperationHelper {
    private:
        friend class Archetype;

        ArchetypeOperationHelper(MemoryManager& memory_manager, const ComponentIdMask& mask);
        ArchetypeOperationHelper() = default;

        static std::vector<ComponentOffset> offsetsFor(const std::vector<ComponentId>& components);

        struct InsertInfo {
            TypeInfo::Constructor constructor;
            ComponentIndex component_index;
        };
        struct DestroyInfo {
            TypeInfo::Destructor destructor;
            ComponentIndex component_index;
        };

        struct InternalMoveInfo {
            TypeInfo::MoveFunction move;
        };

        struct ExternalMoveInfo {
            TypeInfo::Constructor constructor;
            TypeInfo::MoveFunction move;
            ComponentId id;
        };

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        ComponentIndex componentIndex(ComponentId component_id) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (component_id.isNull() || !component_id_to_component_index.has(component_id)) {
                    return ComponentIndex::null();
                }
            }
            return component_id_to_component_index[component_id];
        }

        // NOTE: can be removed?
        ArrayWrapper<ComponentIndex, ComponentId, true> component_id_to_component_index;

        // TODO: use custom allocator
        std::vector<InsertInfo> insert; // only non null init functions
        std::vector<DestroyInfo> destroy; // only non null destroy functions
        ArrayWrapper<ExternalMoveInfo, ComponentIndex, true> external_move;
        ArrayWrapper<InternalMoveInfo, ComponentIndex, true> internal_move; // move or copy function
    };
}
