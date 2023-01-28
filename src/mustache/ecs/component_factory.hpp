#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/id_deff.hpp>

#include <mustache/ecs/component_info.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_handler.hpp>
#include <mustache/utils/default_settings.hpp>

namespace mustache {

    class MUSTACHE_EXPORT ComponentFactory {
    public:
        ComponentFactory() = delete;
        ~ComponentFactory() = delete;

        static bool isEq(const SharedComponentTag* c0,const SharedComponentTag* c1, SharedComponentId id);

        template <typename T>
        static SharedComponentId registerSharedComponent() noexcept {
            static const auto info = ComponentInfo::make<T>();
            static SharedComponentId result = sharedComponentId(info);
            if (!result.isValid()) {
                result = sharedComponentId(info);
            }
            return result;
        }

        template <typename T>
        static ComponentId registerComponent() {
            static const auto info = ComponentInfo::make<T>();
            static ComponentId result = componentId(info);
            if (!result.isValid()) {
                result = componentId(info);
            }
            return result;
        }

        template<typename _C>
        static void applyToMask(ComponentIdMask& mask) noexcept {
            if constexpr (!isComponentShared<_C>()) {
                using Component = typename ComponentType<_C>::type;
                if constexpr (IsComponentRequired<_C>::value) {
                    static const auto id = registerComponent<Component>();
                    mask.set(id, true);
                }
            }
        }

        template<typename _C>
        static void applyToSharedInfo(SharedComponentsInfo& info) noexcept {
            if constexpr (isComponentShared<_C>()) {
                using Component = typename ComponentType<_C>::type;
                if constexpr (IsComponentRequired<_C>::value) {
                    static const auto id = registerSharedComponent<Component>();
                    info.add(id, std::make_shared<_C>());
                }
            }
        }
        template<typename _C>
        static void applyToSharedMask(SharedComponentIdMask& mask) noexcept {
            if constexpr (isComponentShared<_C>()) {
                using Component = typename ComponentType<_C>::type;
                if constexpr (IsComponentRequired<_C>::value) {
                    static const auto id = registerSharedComponent<Component>();
                    mask.add(id);
                }
            }
        }

        template <typename... _C>
        static SharedComponentIdMask makeSharedMask() noexcept {
            SharedComponentIdMask result;
            (applyToSharedMask<_C>(result), ...);
            return result;
        }

        template <typename... _C>
        static SharedComponentsInfo makeSharedInfo() noexcept {
            SharedComponentsInfo result;
            (applyToSharedInfo<_C>(result), ...);
            return result;
        }

        template <typename... _C>
        static ComponentIdMask makeMask() noexcept {
            ComponentIdMask mask;
            (applyToMask<_C>(mask), ...);
            return mask;
        }

        static void initComponents(World&, Entity entity, const ComponentInfo& info, void* data, size_t count);
        static void destroyComponents(World&, Entity entity, const ComponentInfo& info, void* data, size_t count);
        static void moveComponent(World&, Entity entity, const ComponentInfo& info, void* source, void* dest);
        static void copyComponent(World&, Entity entity, const ComponentInfo& info, const void* source, void* dest);

        template<typename T, typename...  ARGS>
        static T* initComponent(void* data, World& world, const Entity& e, ARGS&&... args) {
            const static auto id = registerComponent<T>();
            if constexpr(sizeof...(ARGS) > 0) {
                if constexpr(!std::is_trivially_default_constructible<T>::value) {
                    data = new(data) T {std::forward<ARGS>(args)...};
                }
            } else {
                initComponents(world, e, id, data, 1);
            }
            return static_cast<T*>(data);
        }

        static void initComponents(World& world, Entity entity, ComponentId id, void* data, size_t count) {
            initComponents(world, entity, componentInfo(id), data, count);
        }
        static void destroyComponents(World& world, Entity entity, ComponentId id, void* data, size_t count) {
            destroyComponents(world, entity, componentInfo(id), data, count);
        }
        static void moveComponent(World& world, Entity entity, ComponentId id, void* source, void* dest, bool destroy_source) {
            const auto& info = componentInfo(id);
            moveComponent(world, entity, info, source, dest);
            if (destroy_source) {
                destroyComponents(world, entity, info, source, 1);
            }
        }
        static void copyComponent(World& world, Entity entity, ComponentId id, const void* source, void* dest) {
            copyComponent(world, entity, componentInfo(id), source, dest);
        }

        static ComponentId nextComponentId() noexcept;

        static const ComponentInfo& componentInfo(ComponentId id);
        static ComponentId componentId(const ComponentInfo& info);
        static SharedComponentId sharedComponentId(const ComponentInfo& info);

    };

}
