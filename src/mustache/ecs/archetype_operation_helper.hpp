#pragma once

#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/default_settings.hpp>

#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/component_info.hpp>

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
            MUSTACHE_INLINE void constructor(void* ptr, const Entity& entity, World& world) const noexcept {
                if (constructor_func) {
                    constructor_func(ptr, entity, world);
                }
                if (after_assign_func) {
                    after_assign_func(ptr, entity, world);
                }
            }
            ComponentInfo::Constructor constructor_func;
            ComponentInfo::AfterAssing after_assign_func;
            ComponentIndex component_index;
        };

        struct CreateWithValueInfo {
            const std::byte* value = nullptr;
            size_t size = 0;
            ComponentIndex component_index;
        };

        struct DestroyInfo {
            ComponentInfo::Destructor destructor;
            ComponentIndex component_index;
        };

        struct BeforeRemoveInfo {
            ComponentIndex index;
            ComponentInfo::BeforeRemove function;
        };

        struct InternalMoveInfo {
            ComponentInfo::MoveFunction move_ptr;
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
            ComponentInfo::Constructor constructor_ptr;
            ComponentInfo::AfterAssing after_assign;
            ComponentInfo::MoveFunction move_ptr;
            ComponentId id;
            const std::byte* default_data = nullptr;
            size_t size;

            MUSTACHE_INLINE bool hasConstructorOrAfterAssign() const noexcept {
                return constructor_ptr || default_data != nullptr || after_assign;
            }

            MUSTACHE_INLINE void constructorAndAfterAssign(void* ptr, World& world, const Entity& entity) const {
                if (constructor_ptr) {
                    constructor_ptr(ptr, entity, world);
                } else if (default_data != nullptr) {
                    memcpy(ptr, default_data, size);
                }
                if (after_assign) {
                    after_assign(ptr, entity, world);
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
        std::vector<BeforeRemoveInfo, Allocator<BeforeRemoveInfo> > before_remove_functions; // only non-null beforeRemove functions
        ArrayWrapper<ExternalMoveInfo, ComponentIndex, true> external_move;
        ArrayWrapper<InternalMoveInfo, ComponentIndex, true> internal_move; // move or copy function
    };
}
