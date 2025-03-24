#include "archetype.hpp"

#include <mustache/utils/logger.hpp>
#include <mustache/utils/profiler.hpp>

#include <mustache/ecs/world.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/new_component_data_storage.hpp>
#include <mustache/ecs/default_component_data_storage.hpp>

#include <cstring>

using namespace mustache;

Archetype::Archetype(World& world, ArchetypeIndex id, const ComponentIdMask& mask,
                     const SharedComponentsInfo& shared_components_info, uint32_t chunk_size):
        world_{world},
        mask_{mask},
        shared_components_info_ {shared_components_info},
        operation_helper_{world.memoryManager(), mask},
        version_storage_{world.memoryManager(), mask.componentsCount(), chunk_size, makeComponentMask(mask)},
//        data_storage_{std::make_unique<NewComponentDataStorage>(mask, world_.memoryManager())},
        data_storage_{std::make_unique<DefaultComponentDataStorage>(mask, world_.memoryManager())},
        entities_{world.memoryManager()},
        id_{id} {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
    Logger{}.debug("Archetype version chunk size: %d", chunk_size);
}

Archetype::~Archetype() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
    if (!isEmpty()) {
        Logger{}.error("Destroying non-empty archetype");
    }
    data_storage_->clear(true);
}

ComponentStorageIndex Archetype::pushBack(Entity entity) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);
    const auto index = ComponentStorageIndex::make(entities_.size());
    versionStorage().emplace(worldVersion(), index.toArchetypeIndex());
    entities_.push_back(entity);
    data_storage_->emplace(index);
    return index;
}

ElementView Archetype::getElementView(ArchetypeEntityIndex index) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return ElementView {
            data_storage_->getIterator(ComponentStorageIndex::fromArchetypeIndex(index)),
            *this
    };
}

const ArrayWrapper<Entity, ArchetypeEntityIndex, true>& Archetype::entities() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return entities_;
}

WorldVersion Archetype::getComponentVersion(ArchetypeEntityIndex index, ComponentId id) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return versionStorage().getVersion(versionStorage().chunkAt(index),
                                       getComponentIndex(id));
}

uint32_t Archetype::capacity() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return data_storage_->capacity();
}

ArchetypeIndex Archetype::id() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return id_;
}

uint32_t Archetype::chunkCount() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return static_cast<uint32_t>(entities_.size() / versionStorage().chunkSize());
}

ChunkCapacity Archetype::chunkCapacity() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return ChunkCapacity::make(versionStorage().chunkSize());
}

bool Archetype::isMatch(const ComponentIdMask& mask) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return mask_.isMatch(mask);
}

bool Archetype::isMatch(const SharedComponentIdMask& mask) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return shared_components_info_.isMatch(mask);
}

bool Archetype::hasComponent(ComponentId component_id) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return mask_.has(component_id);
}

bool Archetype::hasComponent(SharedComponentId component_id) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return shared_components_info_.has(component_id);
}

void Archetype::popBack() {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);
    entities_.pop_back();
    data_storage_->decrSize();
}

void Archetype::callDestructor(const ElementView& view) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);
    for (const auto &info : operation_helper_.destroy) {
        info.destructor(view.getData<FunctionSafety::kUnsafe>(info.component_index));
    }
    popBack();
}

void Archetype::callOnRemove(ArchetypeEntityIndex entity_index, const ComponentIdMask& components_to_be_removed) {
    if (operation_helper_.before_remove_functions.empty()) {
        return;
    }
    auto view = getElementView(entity_index);
    for (const auto& [index, function] : operation_helper_.before_remove_functions) {
        const auto id = operation_helper_.component_index_to_component_id[index];
        if (components_to_be_removed.has(id)) {
            function(view.getData(index), *view.getEntity(), world_);
        }
    }
}

bool Archetype::isEmpty() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return entities_.empty();
}

ChunkIndex Archetype::lastChunkIndex() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    const auto entities_count = entities_.size();
    ChunkIndex result;
    if (entities_count > 0) {
        result = ChunkIndex::make((entities_count - 1u) / versionStorage().chunkSize());
    }
    return result;
}

void Archetype::externalMove(Entity entity, Archetype& prev_archetype, ArchetypeEntityIndex prev_index,
                             const ComponentIdMask& skip_constructor) {
    if (this == &prev_archetype) {
        std::string msg = "Moving from archetype [" + mask_.toString() + "] to itself";
        throw std::runtime_error(msg);
    }
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);

    const auto index = pushBack(entity);

    ComponentIndex component_index = ComponentIndex::make(0);
    const auto source_view = prev_archetype.getElementView(prev_index);
    const auto dest_view = getElementView(index.toArchetypeIndex());
    for (const auto& info : operation_helper_.external_move) {
        auto prev_ptr = source_view.getData<FunctionSafety::kSafe>(prev_archetype.getComponentIndex<FunctionSafety::kSafe>(info.id));
        if (prev_ptr != nullptr) {
            auto component_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
            info.move(component_ptr, prev_ptr);
        }
        else {
            if (info.hasConstructorOrAfterAssign() && !skip_constructor.has(info.id)) {
                auto component_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
                info.constructorAndAfterAssign(component_ptr, world_, entity);
            }
        }
        ++component_index;
    }

    prev_archetype.remove(*source_view.getEntity<FunctionSafety::kUnsafe>(), prev_index, mask_);
    world_.entities().updateLocation(entity, this, index.toArchetypeIndex());
}

ArchetypeEntityIndex Archetype::insert(Entity entity, const ComponentIdMask& skip_constructor) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);
    const auto index = pushBack(entity);

    const bool is_skip_mask_empty = skip_constructor.isEmpty();
    const bool skip_all_constructors = (skip_constructor == mask_);
    if (!skip_all_constructors) {
        const auto view = getElementView(index.toArchetypeIndex());
        for (const auto& info : operation_helper_.insert) {
            const auto& component_id = operation_helper_.component_index_to_component_id[info.component_index];
            if (is_skip_mask_empty || !skip_constructor.has(component_id)) {
                auto component_ptr = view.getData<FunctionSafety::kUnsafe>(info.component_index);
                info.constructor(component_ptr, entity, world_);
            }
        }
        for (const auto& info : operation_helper_.create_with_value) {
            const auto& component_id = operation_helper_.component_index_to_component_id[info.component_index];
            if (is_skip_mask_empty || !skip_constructor.has(component_id)) {
                auto component_ptr = view.getData<FunctionSafety::kUnsafe>(info.component_index);
                memcpy(component_ptr, info.value, info.size);
            }
        }
    }
    versionStorage().emplace(worldVersion(), index.toArchetypeIndex());
    world_.entities().updateLocation(entity, this, index.toArchetypeIndex());
    return index.toArchetypeIndex();
}

void Archetype::cloneEntity(Entity source, Entity dest, ArchetypeEntityIndex src_index, CloneEntityMap& map) {
    const auto dest_index = pushBack(dest).toArchetypeIndex();
    world_.entities().updateLocation(dest, this, dest_index);
    {
        ComponentIndex component_index = ComponentIndex::make(0);
        for (const auto& clone_fn: operation_helper_.clone) {
            auto src_data = getConstComponent<FunctionSafety::kUnsafe>(component_index, src_index);
            auto dst_data = getComponent<FunctionSafety::kUnsafe>(component_index, dest_index);
            clone_fn.clone(dst_data, dest, src_data, source, world_, map);
            ++component_index;
        }
    }

    for (auto&& [component_index, after_clone] : operation_helper_.after_clone) {
        auto src_data = getConstComponent<FunctionSafety::kUnsafe>(component_index, src_index);
        auto dst_data = getComponent<FunctionSafety::kUnsafe>(component_index, dest_index);
        after_clone(dst_data, dest, src_data, source, world_, map);
    }
}

void Archetype::internalMove(ArchetypeEntityIndex source_index, ArchetypeEntityIndex destination_index) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);
    // moving last entity to index
    ComponentIndex component_index = ComponentIndex::make(0);
    auto source_view = getElementView(source_index);
    auto dest_view = getElementView(destination_index);
    for (auto& info : operation_helper_.internal_move) {
        auto source_ptr = source_view.getData<FunctionSafety::kUnsafe>(component_index);
        auto dest_ptr = dest_view.getData<FunctionSafety::kUnsafe>(component_index);
        info.move(dest_ptr, source_ptr);
        ++component_index;
    }

    auto source_entity = *source_view.getEntity<FunctionSafety::kUnsafe>();
    auto& dest_entity = *dest_view.getEntity<FunctionSafety::kUnsafe>();

    const auto world_version = worldVersion();
    setVersion(world_version, versionStorage().chunkAt(source_index));
    setVersion(world_version, versionStorage().chunkAt(destination_index));

    world_.entities().updateLocation(dest_entity, nullptr, ArchetypeEntityIndex::null());
    world_.entities().updateLocation(source_entity, this, destination_index);

    dest_entity = source_entity;

    callDestructor(source_view);
}

const ComponentIdMask& Archetype::componentMask() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return mask_;
}

ComponentIndexMask Archetype::makeComponentMask(const ComponentIdMask& mask) const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    ComponentIndexMask index_mask;
    mask.forEachItem([&index_mask, this](ComponentId id) {
        const auto index = getComponentIndex(id);
        if (index.isValid()) {
            index_mask.set(index, true);
        }
    });
    return index_mask;
}

void Archetype::remove(Entity entity_to_destroy, ArchetypeEntityIndex entity_index, const ComponentIdMask& skip_on_remove_call) {
    MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);
//    Logger{}.debug("Removing entity from: %s pos: %d", mask_.toString(), entity_index.toInt());

    callOnRemove(entity_index, mask_.intersection(skip_on_remove_call.inverse()));

    const auto last_index = data_storage_->lastItemIndex().toArchetypeIndex();
    if (entity_index == last_index) {
        if (!operation_helper_.destroy.empty()) {
            callDestructor(getElementView(entity_index));
        } else {
            popBack();
        }
        setVersion(worldVersion(), versionStorage().chunkAt(entity_index));
        world_.entities().updateLocation(entity_to_destroy, nullptr, ArchetypeEntityIndex::null());
    } else {
        internalMove(last_index, entity_index);
    }

    /// TODO: fix for NewComponentDataStorage
    /*if (isEmpty()) {
        entities_.clear();
        entities_.shrink_to_fit();
        data_storage_->clear(true);
    }*/
}

WorldVersion Archetype::worldVersion() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
    return world_.version();
}

void Archetype::clear() {
    MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
    if (isEmpty()) {
        return;
    }

    for (const auto& info : operation_helper_.destroy) {
        for (auto i = ComponentStorageIndex::make(0); i < data_storage_->lastItemIndex().next(); ++i) {
            auto component_ptr = data_storage_->getData(info.component_index, i);
            info.destructor(component_ptr);
        }
    }

    entities_.clear();
    data_storage_->clear(false);
}
