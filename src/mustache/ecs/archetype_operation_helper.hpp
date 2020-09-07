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

    template<typename _Derived>
    struct TArchetypeOperationHelper {
        using SeftType = _Derived;

        template<typename T, FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE ComponentIndex componentIndex() const noexcept {
            static const auto component_id = ComponentFactory::registerComponent<T>();
            return self().template componentIndex<_Safety>(component_id);
        }

        template<typename T, FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE T* getComponent(const ArchetypeInternalEntityLocation& location) const noexcept {
            return reinterpret_cast<T*>(self().template getComponent<_Safety>(componentIndex<T>(), location));
        }

    private:
        MUSTACHE_INLINE SeftType& self() noexcept {
            return *static_cast<SeftType*>(this);
        }
        MUSTACHE_INLINE const SeftType& self() const noexcept {
            return *static_cast<const SeftType*>(this);
        }
    };

    struct ArchetypeOperationHelper : public TArchetypeOperationHelper<ArchetypeOperationHelper> {
        using TArchetypeOperationHelper::getComponent;
        using TArchetypeOperationHelper::componentIndex;

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
                    location.index.toInt() >= capacity) {
                    return nullptr;
                }
            }
            return location.chunk->dataPointerWithOffset<Entity>(entity_offset) + location.index.toInt();
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponent(ComponentIndex component_index, const ArchetypeInternalEntityLocation& location) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (component_index.isNull() || get.size() <= component_index.toInt() ||
                    location.chunk == nullptr || !location.index.isValid() ||
                    location.index.toInt() >= get.capacity()) {
                    return nullptr;
                }
            }
            const auto& info = get[component_index.toInt()];
            const auto offset = info.offset.add(info.size * location.index.toInt());
            return location.chunk->dataPointerWithOffset(offset);
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        ComponentIndex componentIndex(ComponentId component_id) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (component_id.isNull() || component_id.toInt() >= component_id_to_component_index.size()) {
                    return ComponentIndex::null();
                }
            }
            return component_id_to_component_index[component_id.toInt()];
        }

        void updateComponentsVersion(uint32_t world_version, Chunk& chunk) const noexcept {
            auto version_ptr = chunk.dataPointerWithOffset<uint32_t >(ComponentOffset::make(0));
            for (uint32_t i = 0; i < num_components; ++i) {
                version_ptr[i] = world_version;
            }
        }

        // NOTE: can be removed?
        ArrayWrapper<std::vector<ComponentId>, ComponentIndex> component_index_to_component_id;
        std::vector<ComponentIndex> component_id_to_component_index;
        std::vector<GetComponentInfo> get; // ComponentIndex -> {offset, size}
        std::vector<InsertInfo> insert; // only non null init functions
        std::vector<DestroyInfo> destroy; // only non null destroy functions
        std::vector<ExternalMoveInfo> external_move;
        std::vector<InternalMoveInfo> internal_move; // move or copy function
        uint32_t num_components;
        uint32_t capacity;
        ComponentOffset entity_offset;
        ComponentOffset version_offset;
    };
}
