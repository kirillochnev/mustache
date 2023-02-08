#pragma once

#include <mustache/ecs/entity.hpp>

namespace mustache {

    template <typename _Component>
    struct ComponentAssingEvent {
        Entity entity;
        const _Component* component;
    };

    template <typename _Component>
    struct ComponentRemoveEvent {
        Entity entity;
        const _Component* component;
    };

}
