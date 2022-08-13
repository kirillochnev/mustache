#include "world_storage.hpp"

#include <mustache/utils/logger.hpp>
#include <mustache/utils/profiler.hpp>

#include <map>

using namespace mustache;

namespace {
    std::map<std::string, SingletonId> type_map;
    SingletonId next_singleton_id{SingletonId::make(0)};

}

WorldStorage::WorldStorage(MemoryManager& manager):
    singletons_{manager} {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
}

SingletonId WorldStorage::getSingletonId(const std::string& name) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    const auto find_res = type_map.find(name);
    if(find_res != type_map.end()) {
        return find_res->second;
    }
    type_map[name] = {
            next_singleton_id
    };
    Logger{}.debug("New singleton: %s, id: %d", name, next_singleton_id.toInt());
    const auto return_value = next_singleton_id;
    ++next_singleton_id;
    return return_value;
}
