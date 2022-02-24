#include "component_mask.hpp"

#include <mustache/utils/profiler.hpp>

#include <mustache/ecs/component_factory.hpp>

using namespace mustache;

std::string ComponentIdMask::toString() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_2("ComponentIdMask::toString");
    std::string res;
    bool first = true;
    forEachItem([&first, &res](ComponentId id) {
        res += (first ? "[" : ", [") + ComponentFactory::componentInfo(id).name + "]";
        first = false;
    });
    return res;
}
