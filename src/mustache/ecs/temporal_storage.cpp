#include "temporal_storage.hpp"
#include <mustache/utils/profiler.hpp>

using namespace mustache;

TemporalStorage::~TemporalStorage() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
    for (const auto& action : actions_) {
        if (action.action == Action::kCreateEntity &&
            action.type_info != nullptr && action.ptr != nullptr
            && action.type_info->functions.destroy != nullptr) {
            action.type_info->functions.destroy(action.ptr);
        }
    }
    clear();
}

void* TemporalStorage::assignComponent(World& world, Entity entity, ComponentId id, bool skip_constructor) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    const auto& component_info = ComponentFactory::instance().componentInfo(id);
    auto& command = emplaceItem(entity, Action::kAssignComponent);
    command.type_info = &component_info;
    command.component_id = id;
    command.ptr = allocate(static_cast<uint32_t>(component_info.size));
    if (!skip_constructor && component_info.functions.create) {
        component_info.functions.create(command.ptr, entity, world);
    }
    return command.ptr;
}

void TemporalStorage::create(Entity entity, const ComponentIdMask& mask, const SharedComponentsInfo& shared) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    auto& command = emplaceItem(entity, Action::kCreateEntity);
    //command.archetype = archetype;
    if (!mask.isEmpty() || !shared.empty()) {
        command.create_action_index = CreateActionIndex::make(create_actions_.size());
        auto& create_action = create_actions_.emplace_back();
        create_action.mask = mask;
        create_action.shared = shared;
    }
}

void TemporalStorage::removeComponent(Entity entity, ComponentId id) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    auto& command = emplaceItem(entity, Action::kRemoveComponent);
    command.component_id = id;
}

void TemporalStorage::destroy(Entity entity) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    emplaceItem(entity, Action::kDestroyEntity);
}

void TemporalStorage::destroyNow(Entity entity) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    emplaceItem(entity, Action::kDestroyEntityNow);
}

void TemporalStorage::clear() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
    actions_.clear();
    create_actions_.clear();
    if (chunks_.size() > 1u) {
        chunks_.clear();
    } else {
        if (!chunks_.empty()) {
            chunks_.front().free_space = chunks_.front().capacity;
        }
    }
    target_chunk_size_ = total_size_;
    total_size_ = 0u;
}

std::byte* TemporalStorage::allocate(uint32_t size) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    if (chunks_.empty() || chunks_.back().free_space < size) {
        if (target_chunk_size_ < size) {
            target_chunk_size_ = size;
        }
        chunks_.emplace_back(target_chunk_size_);
    }
    auto& chunk = chunks_.back();
    const auto offset = chunk.capacity - chunk.free_space;
    chunk.free_space -= size;
    total_size_ += size;
    return chunk.data.get() + offset;
}

TemporalStorage::ActionInfo& TemporalStorage::emplaceItem(Entity entity, Action action) {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return actions_.emplace_back(entity, action);
}

