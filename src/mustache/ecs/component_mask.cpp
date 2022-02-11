#include "component_mask.hpp"

#include <mustache/ecs/component_factory.hpp>

using namespace mustache;

std::string ComponentIdMask::toString() const noexcept {
    std::string res;
    bool first = true;
    forEachItem([&first, &res](ComponentId id) {
        res += (first ? "[" : ", [") + ComponentFactory::componentInfo(id).name + "]";
        first = false;
    });
    return res;
}
