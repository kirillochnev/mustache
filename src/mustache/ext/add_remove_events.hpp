//
// Created by kirill on 15.04.25.
//

#pragma once

#include <mustache/ecs/world.hpp>
#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/event_manager.hpp>

namespace mustache::ext {
    template<typename _ComponentType>
    struct ComponentAssignEvent {
        using ComponentType = _ComponentType;
        Entity entity;
        _ComponentType* component = nullptr;
    };
    template<typename _ComponentType>
    struct ComponentRemovedEvent {
        using ComponentType = _ComponentType;
        Entity entity;
    };

    struct CommonComponentAssignedEvent {
        Entity entity;
        ComponentId component_id;
        void* component_ptr = nullptr;
    };

    struct CommonComponentRemovedEvent {
        Entity entity;
        ComponentId component_id;
    };


    template<typename _Component, bool _EmitAssignEvent = true, bool _EmitRemoveEvent = true>
    struct ComponentWithNotify;

    template<typename _Component>
    struct ComponentWithNotify<_Component, false, false> {};

    template<typename _Component>
    struct ComponentWithNotify<_Component, false, true> {
        static void beforeRemove(Entity entity, World& world) {
            world.events().post<ComponentRemovedEvent<_Component> >({.entity = entity});
        }
    };

    template<typename _Component>
    struct ComponentWithNotify<_Component, true, false> {
        static void afterAssign(_Component* self, Entity entity, World& world) {
            world.events().post<ComponentAssignEvent<_Component> >({.entity = entity, .component = self});
        }
    };

    template<typename _Component>
    struct ComponentWithNotify<_Component, true, true> {
        static void afterAssign(_Component* self, Entity entity, World& world) {
            world.events().post<ComponentAssignEvent<_Component> >({.entity = entity, .component = self});
        }
        static void beforeRemove(Entity entity, World& world) {
            world.events().post<ComponentRemovedEvent<_Component> >({.entity = entity});
        }
    };

    template<typename _Component, bool _EmitAssignEvent = true, bool _EmitRemoveEvent = true>
    struct ComponentWithNotifyCommon;

    template<typename _Component>
    struct ComponentWithNotifyCommon<_Component, false, false> {};

    template<typename _Component>
    struct ComponentWithNotifyCommon<_Component, false, true> {
        static void beforeRemove(Entity entity, World& world) {
            static const auto id = ComponentFactory::instance().registerComponent<_Component>();
            world.events().post<CommonComponentRemovedEvent>({.entity = entity, .component_id = id});
        }
    };

    template<typename _Component>
    struct ComponentWithNotifyCommon<_Component, true, false> {
        static void afterAssign(_Component* self, Entity entity, World& world) {
            static const auto id = ComponentFactory::instance().registerComponent<_Component>();
            world.events().post<CommonComponentAssignedEvent>({.entity = entity, .component_id = id, .component_ptr = self});
        }
    };

    template<typename _Component>
    struct ComponentWithNotifyCommon<_Component, true, true> {
        static void beforeRemove(Entity entity, World& world) {
            static const auto id = ComponentFactory::instance().registerComponent<_Component>();
            world.events().post<CommonComponentRemovedEvent>({.entity = entity, .component_id = id});
        }
        static void afterAssign(_Component* self, Entity entity, World& world) {
            static const auto id = ComponentFactory::instance().registerComponent<_Component>();
            world.events().post<CommonComponentAssignedEvent>({.entity = entity, .component_id = id, .component_ptr = self});
        }
    };
}
