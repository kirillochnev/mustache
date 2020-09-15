#pragma once

#include <mustache/ecs/chunk.hpp>
#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_handler.hpp>
#include <mustache/utils/type_info.hpp>
#include <mustache/utils/default_settings.hpp>

namespace mustache {

    class Context;
    class Chunk;

    class ComponentFactory {
    public:
        ComponentFactory() = delete;
        ~ComponentFactory() = delete;

        template <typename T>
        static ComponentId registerComponent() {
            static const auto info = makeTypeInfo<T>();
            static ComponentId result = componentId(info);
            if(!result.isValid()) {
                result = componentId(info);
            }
            return result;
        }

        template<typename _C>
        static void applyToMask(ComponentMask& mask) noexcept {
            using Component = typename ComponentType<_C>::type;
            if constexpr (IsComponentRequired<_C>::value) {
                static const auto id = registerComponent< typename ComponentType<_C>::type >();
                mask.set(id, true);
            }
        }
        template <typename... _C>
        static ComponentMask makeMask() noexcept {
            ComponentMask mask;
            (applyToMask<_C>(mask), ...);
            return mask;
        }

        static void initComponents(const TypeInfo& info, void* data, size_t count);
        static void destroyComponents(const TypeInfo& info, void* data, size_t count);
        static void moveComponent(const TypeInfo& info, void* source, void* dest);
        static void copyComponent(const TypeInfo& info, const void* source, void* dest);

        static void initComponents(ComponentId id, void* data, size_t count) {
            initComponents(componentInfo(id), data, count);
        }
        static void destroyComponents(ComponentId id, void* data, size_t count) {
            destroyComponents(componentInfo(id), data, count);
        }
        static void moveComponent(ComponentId id, void* source, void* dest) {
            moveComponent(componentInfo(id), source, dest);
        }
        static void copyComponent(ComponentId id, const void* source, void* dest) {
            copyComponent(componentInfo(id), source, dest);
        }

        static ComponentId nextComponentId() noexcept;

        static const TypeInfo& componentInfo(ComponentId id);
        static ComponentId componentId(const TypeInfo& info);

    };

}
