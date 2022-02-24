#include "temporal_storage.hpp"

using namespace mustache;

void* TemporalStorage::assignComponent(Entity entity, ComponentId id, bool skip_constructor) {
    const auto& component_info = ComponentFactory::componentInfo(id);
    auto& info = actions_.emplace_back();
    info.action = Action::kAssignComponent;
    info.entity = entity;
    info.index = static_cast<uint32_t >(commands_.assign.size());
    auto& command = commands_.assign.emplace_back();
    command.type_info = &component_info;
    command.component_id = id;
    command.ptr = allocate(static_cast<uint32_t >(component_info.size));
    if (!skip_constructor && component_info.functions.create) {
        component_info.functions.create(command.ptr);
    }
    return command.ptr;
}

void TemporalStorage::create(Entity entity, Archetype* archetype) {
    auto& action = actions_.emplace_back();
    action.entity = entity;
    action.index = static_cast<uint32_t >(commands_.create.size());
    action.action = Action::kCreateEntity;
    commands_.create.push_back(archetype);
}

void TemporalStorage::removeComponent(Entity entity, ComponentId id) {
    auto& info = actions_.emplace_back();
    info.action = Action::kRemoveComponent;
    info.entity = entity;
    info.index = static_cast<uint32_t >(commands_.remove.size());
    commands_.remove.emplace_back().component_id = id;
}

void TemporalStorage::destroy(Entity entity) {
    auto& info = actions_.emplace_back();
    info.action = Action::kDestroyEntity;
    info.entity = entity;
}

void TemporalStorage::destroyNow(Entity entity) {
    auto& info = actions_.emplace_back();
    info.action = Action::kDestroyEntityNow;
    info.entity = entity;
}

void TemporalStorage::clear() {
    for (const auto& assign_action : commands_.assign) {
        if (assign_action.type_info != nullptr && assign_action.type_info->functions.destroy != nullptr) {
            assign_action.type_info->functions.destroy(assign_action.ptr);
        }
    }
    actions_.clear();
    commands_.assign.clear();
    commands_.remove.clear();
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
