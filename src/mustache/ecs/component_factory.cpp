#include "component_factory.hpp"
#include <map>
#include <mustache/utils/logger.hpp>

using namespace mustache;

namespace {
    struct Element {
        TypeInfo info{};
        ComponentId id{ComponentId::make(-1)};
    };
    std::map<std::string, Element> type_map;
    ComponentId next_component_id{ComponentId::make(0)};
    std::vector<TypeInfo> components_info;

    template<typename _F>
    MUSTACHE_INLINE static void applyFunction(void* data, _F&& f, size_t count, size_t size) {
        if (f) {
            std::byte *ptr = static_cast<std::byte *>(data);
            for (uint32_t i = 0; i < count; ++i) {
                f(static_cast<void *>(ptr));
                ptr += size;
            }
        }
    }
}

ComponentId ComponentFactory::componentId(const TypeInfo &info) {
    const auto find_res = type_map.find(info.name);
    if(find_res != type_map.end()) {
        if(components_info.size() <= find_res->second.id.toInt()) {
            components_info.resize(find_res->second.id.toInt() + 1);
        }
        components_info[find_res->second.id.toInt()] = find_res->second.info;
        return find_res->second.id;
    }
    type_map[info.name] = {
            info,
            next_component_id
    };
    if(components_info.size() <= next_component_id.toInt()) {
        components_info.resize(next_component_id.toInt() + 1);
    }
    components_info[next_component_id.toInt()] = info;
    Logger{}.debug("New component: %s, id: %d", info.name, next_component_id);
    ComponentId return_value = next_component_id;
    next_component_id = ComponentId::make(next_component_id.toInt() + 1);
    return return_value;
}

const TypeInfo& ComponentFactory::componentInfo(ComponentId id) {
    return components_info[id.toInt()];
}

void ComponentFactory::initComponents(const TypeInfo& info, void* data, size_t count) {
    applyFunction(data, info.functions.create, count, info.size);
}

void ComponentFactory::destroyComponents(const TypeInfo& info, void* data, size_t count) {
    applyFunction(data, info.functions.destroy, count, info.size);
}
