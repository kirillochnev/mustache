#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/id_deff.hpp>

#include <mustache/ecs/component_info.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_handler.hpp>

#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/construct_at.hpp>
#include <mustache/utils/default_settings.hpp>

namespace mustache {

    class MUSTACHE_EXPORT ComponentFactory : public Uncopiable {
    public:

        static ComponentFactory& instance();

        bool isEq(const SharedComponentTag* c0,const SharedComponentTag* c1, SharedComponentId id) const;

        template <typename T>
        SharedComponentId registerSharedComponent() const noexcept {
            static const auto info = ComponentInfo::make<T>();
            static SharedComponentId result = sharedComponentId(info);
            if (!result.isValid()) {
                result = sharedComponentId(info);
            }
            return result;
        }

        template <typename T>
        ComponentId registerComponent() const {
        	static_assert(!std::is_abstract_v<T>);
            static const auto info = ComponentInfo::make<T>();
            static ComponentId result = componentId(info);
            if (!result.isValid()) {
                result = componentId(info);
            }
            return result;
        }

        template<typename _C>
        void applyToMask(ComponentIdMask& mask) const noexcept {
            if constexpr (!isComponentShared<_C>()) {
                using Component = typename ComponentType<_C>::type;
                if constexpr (IsComponentRequired<_C>::value) {
                    static const auto id = registerComponent<Component>();
                    mask.set(id, true);
                }
            }
        }

        template<typename _C>
        void applyToSharedInfo(SharedComponentsInfo& info) const noexcept {
            if constexpr (isComponentShared<_C>()) {
                using Component = typename ComponentType<_C>::type;
                if constexpr (IsComponentRequired<_C>::value) {
                    static const auto id = registerSharedComponent<Component>();
                    info.add(id, std::make_shared<_C>());
                }
            }
        }
        template<typename _C>
        void applyToSharedMask(SharedComponentIdMask& mask) const noexcept {
            if constexpr (isComponentShared<_C>()) {
                using Component = typename ComponentType<_C>::type;
                if constexpr (IsComponentRequired<_C>::value) {
                    static const auto id = registerSharedComponent<Component>();
                    mask.add(id);
                }
            }
        }

        template <typename... _C>
        [[nodiscard]] SharedComponentIdMask makeSharedMask() const noexcept {
            SharedComponentIdMask result;
            (applyToSharedMask<_C>(result), ...);
            return result;
        }

        template <typename... _C>
        [[nodiscard]] SharedComponentsInfo makeSharedInfo() const noexcept {
            SharedComponentsInfo result;
            (applyToSharedInfo<_C>(result), ...);
            return result;
        }

        template <typename... _C>
        [[nodiscard]] ComponentIdMask makeMask() const noexcept {
            ComponentIdMask mask;
            (applyToMask<_C>(mask), ...);
            return mask;
        }

        void initComponents(World&, Entity entity, const ComponentInfo& info, void* data, size_t count) const;
        void destroyComponents(World&, Entity entity, const ComponentInfo& info, void* data, size_t count) const;
        void moveComponent(World&, Entity entity, const ComponentInfo& info, void* source, void* dest) const;
        void copyComponent(World&, Entity entity, const ComponentInfo& info, const void* source, void* dest) const;

        template<typename T, typename...  ARGS>
        T* initComponent(void* data, World& world, const Entity& e, ARGS&&... args) const {
            const static auto id = registerComponent<T>();
            if constexpr(sizeof...(ARGS) > 0) {
                if constexpr(!std::is_trivially_default_constructible<T>::value) {
					data = constructAt<T>(data, std::forward<ARGS>(args)...);
                }
                ComponentInfo::afterComponentAssign<T>(data, e, world);
            } else {
                initComponents(world, e, id, data, 1);
            }
            return static_cast<T*>(data);
        }

        void initComponents(World& world, Entity entity, ComponentId id, void* data, size_t count) const {
            initComponents(world, entity, componentInfo(id), data, count);
        }
        void destroyComponents(World& world, Entity entity, ComponentId id, void* data, size_t count) const {
            destroyComponents(world, entity, componentInfo(id), data, count);
        }
        void moveComponent(World& world, Entity entity, ComponentId id, void* source, void* dest, bool destroy_source) const {
            const auto& info = componentInfo(id);
            moveComponent(world, entity, info, source, dest);
            if (destroy_source) {
                destroyComponents(world, entity, info, source, 1);
            }
        }
        void copyComponent(World& world, Entity entity, ComponentId id, const void* source, void* dest) const {
            copyComponent(world, entity, componentInfo(id), source, dest);
        }

        [[nodiscard]] ComponentId nextComponentId() const noexcept;

        [[nodiscard]] const ComponentInfo& componentInfo(ComponentId id) const;
        [[nodiscard]]  ComponentId componentId(const ComponentInfo& info) const;
        [[nodiscard]] SharedComponentId sharedComponentId(const ComponentInfo& info) const;

    };

}

