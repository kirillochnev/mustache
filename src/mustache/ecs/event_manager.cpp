#include "event_manager.hpp"

#include <mustache/utils/logger.hpp>
#include <mustache/utils/profiler.hpp>
#include <mustache/utils/container_map.hpp>


using namespace mustache;

namespace {
    mustache::map<std::string, EventId> type_map;
    EventId next_event_id = EventId::make(0u);
}

EventId EventManager::registerEventType(const std::string& name) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_1(__FUNCTION__);
    const auto find_res = type_map.find(name);
    if(find_res != type_map.end()) {
        return find_res->second;
    }
    type_map[name] = next_event_id;
    Logger{}.debug("New event type: [%s], id: [%d]", name, next_event_id.toInt());
    const auto result = next_event_id;
    ++next_event_id;
    return result;
}
