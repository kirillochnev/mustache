#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/id_deff.hpp>
#include <mustache/utils/type_info.hpp>
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
            static const auto info = makeTypeInfo<T>();
            static SharedComponentId result = sharedComponentId(info);
            if (!result.isValid()) {
                result = sharedComponentId(info);
            }
            return result;
        }

        template <typename T>
        static ComponentId registerComponent() {
            static const auto info = makeTypeInfo<T>();
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

        static void initComponents(const TypeInfo& info, void* data, size_t count);
        static void destroyComponents(const TypeInfo& info, void* data, size_t count);
        static void moveComponent(const TypeInfo& info, void* source, void* dest);
        static void copyComponent(const TypeInfo& info, const void* source, void* dest);

        template<typename T, typename...  ARGS>
        static T* initComponent(void* data, ARGS&&... args) {
            const static auto id = registerComponent<T>();
            if constexpr(sizeof...(ARGS) > 0) {
                if constexpr(!std::is_trivially_default_constructible<T>::value) {
                    data = new(data) T(std::forward<ARGS>(args)...);
                }
            } else {
                initComponents(id, data, 1);
            }
            return static_cast<T*>(data);
        }

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
        static SharedComponentId sharedComponentId(const TypeInfo& info);

    };

}
