#include "world_storage.hpp"

#include <mustache/utils/logger.hpp>
#include <mustache/utils/profiler.hpp>

#include <map>

using namespace mustache;

namespace {
    struct Element {
        TypeInfo info{};
        SingletonId id{SingletonId::make(-1)};
    };

    std::map<size_t, Element> type_map;
    SingletonId next_singleton_id{SingletonId::make(0)};

}

WorldStorage::WorldStorage(MemoryManager& manager):
    singletons_{manager} {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
}

SingletonId WorldStorage::getSingletonId(const TypeInfo& info) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    const auto find_res = type_map.find(info.type_id_hash_code);
    if(find_res != type_map.end()) {
        return find_res->second.id;
    }
    type_map[info.type_id_hash_code] = {
            info,
            next_singleton_id
    };
    Logger{}.debug("New singleton: %s, id: %d", info.name, next_singleton_id.toInt());
    const auto return_value = next_singleton_id;
    ++next_singleton_id;
    return return_value;
}
