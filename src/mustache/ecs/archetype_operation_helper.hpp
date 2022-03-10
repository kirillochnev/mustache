#pragma once

#include <mustache/utils/type_info.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/default_settings.hpp>

#include <mustache/ecs/id_deff.hpp>

#include <cstring>
#include <vector>

namespace mustache {
    struct ComponentIdMask;

    class MUSTACHE_EXPORT ArchetypeOperationHelper {
    private:
        friend class Archetype;

        ArchetypeOperationHelper(MemoryManager& memory_manager, const ComponentIdMask& mask);
        ArchetypeOperationHelper() = default;

        static std::vector<ComponentOffset> offsetsFor(const std::vector<ComponentId>& components);

        struct InsertInfo {
            TypeInfo::Constructor constructor;
            ComponentIndex component_index;
        };

        struct CreateWithValueInfo {
            const std::byte* value = nullptr;
            size_t size = 0;
            ComponentIndex component_index;
        };

        struct DestroyInfo {
            TypeInfo::Destructor destructor;
            ComponentIndex component_index;
        };

        struct InternalMoveInfo {
            TypeInfo::MoveFunction move_ptr;
            size_t size;
            MUSTACHE_INLINE void move(void* dest, void* src) const {
                if (move_ptr) {
                    move_ptr(dest, src);
                } else {
                    memcpy(dest, src, size);
                }
            }
        };

        struct ExternalMoveInfo {
            TypeInfo::Constructor constructor_ptr;
            TypeInfo::MoveFunction move_ptr;
            ComponentId id;
            const std::byte* default_data = nullptr;
            size_t size;

            MUSTACHE_INLINE bool hasConstructor() const noexcept {
                return constructor_ptr || default_data != nullptr;
            }

            MUSTACHE_INLINE void constructor(void* ptr, World& world, const Entity& entity) const {
                if (constructor_ptr) {
                    constructor_ptr(ptr, entity, world);
                } else {
                    memcpy(ptr, default_data, size);
                }
            }
            MUSTACHE_INLINE void move(void* dest, void* source) const {
                if (move_ptr) {
                    move_ptr(dest, source);
                } else {
                    memcpy(dest, source, size);
                }
            }
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
        ArrayWrapper<ComponentId, ComponentIndex, true> component_index_to_component_id;

        std::vector<InsertInfo, Allocator<InsertInfo> > insert; // only non-null init functions
        std::vector<CreateWithValueInfo, Allocator<CreateWithValueInfo> > create_with_value; // only non-empty values
        std::vector<DestroyInfo, Allocator<DestroyInfo> > destroy; // only non-null destroy functions
        ArrayWrapper<ExternalMoveInfo, ComponentIndex, true> external_move;
        ArrayWrapper<InternalMoveInfo, ComponentIndex, true> internal_move; // move or copy function
    };
}
