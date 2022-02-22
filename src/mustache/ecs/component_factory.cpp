#include "component_factory.hpp"

#include <mustache/utils/logger.hpp>

#include <map>
#include <mutex>

using namespace mustache;

namespace {

    template <typename IdType>
    struct ComponentIdStorage {
        struct Element {
            TypeInfo info{};
            IdType id = IdType::null();
        };
        std::map<std::string, Element> type_map;
        IdType next_component_id{IdType::make(0)};
        std::vector<TypeInfo> components_info;
        mutable std::mutex mutex;

        IdType getId(const TypeInfo& info) {
            std::unique_lock lock {mutex};
            const auto find_res = type_map.find(info.name);

            if (find_res != type_map.end()) {
                if(components_info.size() <= find_res->second.id.toInt()) {
                    components_info.resize(find_res->second.id.toInt() + 1);
                }
                components_info[find_res->second.id.toInt()] = find_res->second.info;
                return find_res->second.id;
            }
            if (!info.default_value.empty() && info.default_value.size() != info.size) {
                throw std::runtime_error("Invalid component default value for component: " + info.name);
            }
            type_map[info.name] = {
                    info,
                    next_component_id
            };
            if (components_info.size() <= next_component_id.toInt()) {
                components_info.resize(next_component_id.toInt() + 1);
            }
            components_info[next_component_id.toInt()] = info;
            Logger{}.debug("New component: %s, id: %d", info.name, next_component_id.toInt());
            IdType return_value = next_component_id;
            ++next_component_id;
            return return_value;
        }
        const TypeInfo& componentInfo(IdType id) const noexcept {
            std::unique_lock lock {mutex};
            return components_info[id.toInt()];
        }

    };

    ComponentIdStorage<ComponentId> component_id_storage;
    ComponentIdStorage<SharedComponentId> shared_component_id_storage;

    template<typename _F>
    static void applyFunction(void* data, _F&& f, size_t count, size_t size) {
        if (f) {
            std::byte *ptr = static_cast<std::byte *>(data);
            for (uint32_t i = 0; i < count; ++i) {
                f(static_cast<void *>(ptr));
                ptr += size;
            }
        }
    }
}

SharedComponentId ComponentFactory::sharedComponentId(const TypeInfo& info) {
    return shared_component_id_storage.getId(info);
}

ComponentId ComponentFactory::componentId(const TypeInfo& info) {
    return component_id_storage.getId(info);
}

const TypeInfo& ComponentFactory::componentInfo(ComponentId id) {
    return component_id_storage.componentInfo(id);
}

void ComponentFactory::initComponents(const TypeInfo& info, void* data, size_t count) {
    applyFunction(data, info.functions.create, count, info.size);
}

void ComponentFactory::destroyComponents(const TypeInfo& info, void* data, size_t count) {
    applyFunction(data, info.functions.destroy, count, info.size);
}

ComponentId ComponentFactory::nextComponentId() noexcept {
    return component_id_storage.next_component_id;
}

bool ComponentFactory::isEq(const SharedComponentTag* c0,const SharedComponentTag* c1, SharedComponentId id) {
    return shared_component_id_storage.componentInfo(id).functions.compare(c0, c1);
}

void ComponentFactory::moveComponent(const TypeInfo& info, void* source, void* dest) {
    if (!info.functions.move) {
        throw std::runtime_error("Invalid move function for: " + info.name);
    }
    info.functions.move(dest, source);
}

void ComponentFactory::copyComponent(const TypeInfo& info, const void* source, void* dest) {
    if (!info.functions.copy) {
        throw std::runtime_error("Invalid copy function for: " + info.name);
    }
    info.functions.copy(dest, source);
}
