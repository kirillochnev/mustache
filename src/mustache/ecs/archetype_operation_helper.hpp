#pragma once

#include <mustache/utils/type_info.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <mustache/utils/default_settings.hpp>

#include <mustache/ecs/chunk.hpp>
#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <vector>

namespace mustache {

    struct ArchetypeInternalEntityLocation {
        Chunk* chunk;
        ChunkEntityIndex index;
    };

    class ArchetypeOperationHelper {
    private:
        friend class Archetype;
        template<typename T, FunctionSafety _Safety = FunctionSafety::kDefault>
        ComponentIndex componentIndex() const noexcept {
            static const auto component_id = ComponentFactory::registerComponent<T>();
            return componentIndex<_Safety>(component_id);
        }

        template<typename T, FunctionSafety _Safety = FunctionSafety::kDefault>
        T* getComponent(const ArchetypeInternalEntityLocation& location) const noexcept {
            return reinterpret_cast<T*>(getComponent<_Safety>(componentIndex<T>(), location));
        }

        explicit ArchetypeOperationHelper(const ComponentMask& mask);
        ArchetypeOperationHelper() = default;

        static std::vector<ComponentOffset> offsetsFor(const std::vector<ComponentId>& components);

        struct InsertInfo {
            ComponentOffset offset;
            uint32_t component_size;
            TypeInfo::Constructor constructor;
        };
        struct DestroyInfo {
            ComponentOffset offset;
            uint32_t component_size;
            TypeInfo::Destructor destructor;
        };

        struct InternalMoveInfo {
            ComponentOffset offset;
            uint32_t component_size;
            TypeInfo::MoveFunction move;
        };

        struct ExternalMoveInfo {
            TypeInfo::Constructor constructor;
            TypeInfo::MoveFunction move;
            ComponentOffset offset;
            uint32_t size;
            ComponentId id;
        };

        struct GetComponentInfo {
            ComponentOffset offset;
            uint32_t size;
        };

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        Entity* getEntity(const ArchetypeInternalEntityLocation& location) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (location.chunk == nullptr || !location.index.isValid() ||
                    location.index > index_of_last_entity_in_chunk) {
                    return nullptr;
                }
            }
            return location.chunk->dataPointerWithOffset<Entity>(entity_offset) + location.index.toInt();
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponent(ComponentIndex component_index, const ArchetypeInternalEntityLocation& location) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (component_index.isNull() || !get.has(component_index) ||
                    location.chunk == nullptr || !location.index.isValid() ||
                    location.index > index_of_last_entity_in_chunk) {
                    return nullptr;
                }
            }
            const auto& info = get[component_index];
            const auto offset = info.offset.add(info.size * location.index.toInt());
            return location.chunk->dataPointerWithOffset(offset);
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        ComponentIndex componentIndex(ComponentId component_id) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (component_id.isNull() || !component_id_to_component_index.has(component_id)) {
                    return ComponentIndex::null();
                }
            }
            return component_id_to_component_index[component_id];
        }

        void updateComponentsVersion(WorldVersion world_version, Chunk& chunk) const noexcept {
            auto version_ptr = chunk.dataPointerWithOffset<WorldVersion >(ComponentOffset::make(0));
            for (uint32_t i = 0; i < num_components; ++i) {
                version_ptr[i] = world_version;
            }
        }

        uint32_t chunkCapacity() const noexcept {
            return index_of_last_entity_in_chunk.next().toInt();
        }
        // NOTE: can be removed?
        ArrayWrapper<std::vector<ComponentId>, ComponentIndex> component_index_to_component_id;
        ArrayWrapper<std::vector<ComponentIndex>, ComponentId> component_id_to_component_index;
        ArrayWrapper<std::vector<GetComponentInfo>, ComponentIndex> get; // ComponentIndex -> {offset, size}
        std::vector<InsertInfo> insert; // only non null init functions
        std::vector<DestroyInfo> destroy; // only non null destroy functions
        ArrayWrapper<std::vector<ExternalMoveInfo>, ComponentIndex> external_move;
        ArrayWrapper<std::vector<InternalMoveInfo>, ComponentIndex> internal_move; // move or copy function
        uint32_t num_components;
        ChunkEntityIndex index_of_last_entity_in_chunk;
        ComponentOffset entity_offset;
        ComponentOffset version_offset;
    };
}
