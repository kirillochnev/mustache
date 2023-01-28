#include <mustache/ecs/component_info.hpp>
#include <stdexcept>

using namespace mustache;

void ComponentInfo::error(const char* msg) {
    throw std::runtime_error(msg);
}
