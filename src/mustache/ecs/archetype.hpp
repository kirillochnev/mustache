#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache/ecs/archetype_operation_helper.hpp>
#include <mustache/ecs/base_component_data_storage.hpp>

#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/entity_group.hpp>
#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/type_info.hpp>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace mustache {

    class World;
    class EntityManager;
    class Archetype;

    // NOTE: element view does not update component versions
    struct ElementView : public DataStorageIterator {
        using DataStorageIterator::DataStorageIterator;

        ElementView(const DataStorageIterator& view, const Archetype& archetype):
                DataStorageIterator{view},
                archetype_{&archetype} {

        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE Entity* getEntity() const;


        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE void* getSharedComponent(SharedComponentIndex index) const noexcept;

        const Archetype* archetype_ = nullptr;
    };

    struct ArchetypeFilterParam {
        std::vector<ComponentIndex> components;
        WorldVersion version;
    };

    struct ArchetypeComponents {
        ComponentIdMask unique;
        SharedComponentIdMask shared;
    };
    /**
     * Stores Entities with same component set
     * NOTE: It is has no information about entity manager, so Archetype's methods don't effects entity location.
     */
    class Archetype : public Uncopiable {
    public:
        Archetype(World& world, ArchetypeIndex id, const ComponentIdMask& mask,
                  const SharedComponentsInfo& shared_components_info, uint32_t chunk_size);
        ~Archetype();

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        Entity entityAt(ArchetypeEntityIndex index) const {
            if constexpr (isSafe(_Safety)) {
                if (!index.isValid() || index.toInt() >= entities_.size()) {
                    return Entity{};
                }
            }
            return entities_[index];
        }

        [[nodiscard]] EntityGroup createGroup(size_t count);

        [[nodiscard]] uint32_t size() const noexcept {
            return static_cast<uint32_t>(entities_.size());
        }

        [[nodiscard]] uint32_t capacity() const noexcept {
            return data_storage_->capacity();
        }

        [[nodiscard]] ArchetypeIndex id() const noexcept {
            return id_;
        }

        [[nodiscard]] uint32_t chunkCount() const noexcept {
            return static_cast<uint32_t>(entities_.size() / chunk_size_);
        }

        [[nodiscard]] ChunkCapacity chunkCapacity() const noexcept {
            return ChunkCapacity::make(chunk_size_);
        }

        [[nodiscard]] bool isMatch(const ComponentIdMask& mask) const noexcept {
            return mask_.isMatch(mask);
        }

        [[nodiscard]] bool isMatch(const SharedComponentIdMask& mask) const noexcept {
            return shared_components_info_.ids.isMatch(mask);
        }

        [[nodiscard]] SharedComponentTag* getSharedComponent(SharedComponentId id) const noexcept {
            uint32_t i = 0u;
            SharedComponentTag* result = nullptr;
            shared_components_info_.ids.forEachItem([this, id, &i, &result](SharedComponentId current_id){
                if (current_id == id) {
                    result = shared_components_info_.data[i].get();
                    return false;
                }
                ++i;
                return true;
            });
            return result;
        }

        bool hasComponent(ComponentId component_id) const noexcept {
            return mask_.has(component_id);
        }

        bool hasComponent(SharedComponentId component_id) const noexcept {
            return shared_components_info_.ids.has(component_id);
        }

        template<typename T>
        bool hasComponent() const noexcept {
            static const auto component_id = ComponentFactory::registerComponent<T>();
            return mask_.has(component_id);
        }

        WorldVersion worldVersion() const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] ComponentIndex getComponentIndex(ComponentId id) const noexcept {
            return operation_helper_.componentIndex<_Safety>(id);
        }

        [[nodiscard]] const ComponentIdMask& componentMask() const noexcept {
            return mask_;
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponent(ComponentId component_id, ArchetypeEntityIndex index) const noexcept {
            return data_storage_->getData<_Safety>(operation_helper_.componentIndex(component_id),
                    ComponentStorageIndex::fromArchetypeIndex(index));
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        const void* getConstComponent(ComponentIndex component_index, ArchetypeEntityIndex index) const noexcept {
            return data_storage_->getData<_Safety>(component_index, ComponentStorageIndex::fromArchetypeIndex(index));
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponent(ComponentIndex component_index, ArchetypeEntityIndex index, WorldVersion version) noexcept {
            auto res = data_storage_->getData<_Safety>(component_index, ComponentStorageIndex::fromArchetypeIndex(index));
            if (res != nullptr) {
                const auto chunk = index.toInt() / chunk_size_;
                auto versions = chunk_versions_.data() + components_count_ * chunk;
                global_versions_[component_index] = version;
                versions[component_index.toInt()] = version;
            }

            return res;
        }

        [[nodiscard]] ElementView getElementView(ArchetypeEntityIndex index) const noexcept {
            return ElementView {
                    data_storage_->getIterator(ComponentStorageIndex::fromArchetypeIndex(index)),
                *this
            };
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] WorldVersion getComponentVersion(ChunkIndex index, ComponentIndex component_index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (components_count_ <= component_index.toInt() ||
                    chunk_versions_.size() / components_count_ <= index.toInt()) {
                    return WorldVersion::null();
                }
            }
            return chunk_versions_[versionIndex(index, component_index)];
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] WorldVersion getComponentVersion(ArchetypeEntityIndex entity_index,
                ComponentId id) const noexcept {
            const auto chunk_index = ChunkIndex::make(entity_index.toInt() / chunk_size_);
            const auto component_index = getComponentIndex<_Safety>(id);
            return getComponentVersion<_Safety>(chunk_index, component_index);
        }

        [[nodiscard]] ChunkIndex lastChunkIndex() const noexcept {
            const auto entities_count = entities_.size();
            ChunkIndex result;
            if (entities_count > 0) {
                result = ChunkIndex::make((entities_count - 1u) / chunk_size_);
            }
            return result;
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        bool updateGlobalComponentVersion(const ArchetypeFilterParam& check, const ArchetypeFilterParam& set) noexcept {
            bool need_update = check.version.isNull() || check.components.empty();
            for (auto index : check.components) {
                if constexpr (isSafe(_Safety)) {
                    if (!global_versions_.has(index)) {
                        continue;
                    }
                }
                if (global_versions_[index] > check.version) {
                    need_update = true;
                    break;
                }
            }
            if (need_update) {
                for (auto index : set.components) {
                    if constexpr (isSafe(_Safety)) {
                        if (!global_versions_.has(index)) {
                            continue;
                        }
                    }
                    global_versions_[index] = set.version;
                }
            }
            return need_update;
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        bool updateChunkComponentVersions(const ArchetypeFilterParam& check,
                                          const ArchetypeFilterParam& set, ChunkIndex chunk) noexcept {
            const auto first_index = components_count_ * chunk.toInt();
            if constexpr (isSafe(_Safety)) {
                const auto last_index = first_index + components_count_;
                if (!chunk.isValid() || !set.version.isValid() || chunk_versions_.size() <= last_index) {
                    return false;
                }
            }
            auto versions = chunk_versions_.data() + first_index;
            bool result = check.version.isNull() || check.components.empty();
            if (!result) {
                for (auto component_index : check.components) {
                    if (versions[component_index.toInt()] > check.version) {
                        result = true;
                        break;
                    }
                }
            }

            if (result) {
                for (auto component_index : set.components) {
                    versions[component_index.toInt()] = set.version;
                }
            }
            return result;
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] ComponentIndexMask makeComponentMask(const ComponentIdMask& mask) const noexcept {
            ComponentIndexMask index_mask;
            mask.forEachItem([&index_mask, this](ComponentId id) {
                const auto index = getComponentIndex(id);
                const auto value = !isSafe(_Safety) || index.isValid();
                index_mask.set(index, value);
            });
            return index_mask;
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] MUSTACHE_INLINE void* getSharedComponent(SharedComponentIndex index) const noexcept {
            return nullptr; // TODO: impl me
        }

        template<typename T>
        [[nodiscard]] MUSTACHE_INLINE SharedComponentIndex sharedComponentIndex() const noexcept {
            return SharedComponentIndex::make(0); // TODO: impl me
        }
        template<typename... ARGS>
        bool getSharedComponents(std::tuple<ARGS...>& out) {
            out = std::make_tuple(static_cast<ARGS*>(getSharedComponent(sharedComponentIndex<ARGS>()))...);
            return true;
        }

        [[nodiscard]] MUSTACHE_INLINE const SharedComponentsInfo& sharedComponentInfo() const noexcept {
            return shared_components_info_;
        }
    private:
        friend ElementView;
        friend EntityManager;

        // NOTE: can resize chunk_versions_
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void updateAllVersions(ArchetypeEntityIndex index, WorldVersion world_version) noexcept {
            if (!componentMask().isEmpty()) {
                if constexpr (isSafe(_Safety)) {
                    if (!entities_.has(index) || !index.isValid() || !world_version.isValid()) {
                        return;
                    }
                }
                const auto first_index = components_count_ * index.toInt() / chunk_size_;
                const auto last_index = first_index + components_count_;

                if (chunk_versions_.size() <= last_index) {
                    chunk_versions_.resize(last_index);
                }
                for (uint32_t component_i = 0; component_i < components_count_; ++component_i) {
                    chunk_versions_[component_i + first_index] = world_version;
                    global_versions_[ComponentIndex::make(component_i)] = world_version;
                }
            }
        }

        MUSTACHE_INLINE uint32_t versionIndex(ChunkIndex chunk_index, ComponentIndex component_index) const noexcept {
            return chunk_index.toInt() * components_count_ + component_index.toInt();
        }

        template<typename _F>
        void forEachEntity(_F&& callback) {
            if (isEmpty()) {
                return;
            }
            auto view = getElementView(ArchetypeEntityIndex::make(0));
            while (view.isValid()) {
                callback(*view.getEntity<FunctionSafety::kUnsafe>());
                ++view;
            }
        }


        [[nodiscard]] ComponentStorageIndex pushBack(Entity entity) {
            const auto index = ComponentStorageIndex::make(entities_.size());
            entities_.push_back(entity);
            data_storage_->pushBack();
            return index;
        }

        void popBack() {
            entities_.pop_back();
            data_storage_->decrSize();
        }

        /// Archetype size == 0
        [[nodiscard]] bool isEmpty() const noexcept {
            return entities_.empty();
        }

        /// Archetype has not components
        [[nodiscard]] bool isNull() const noexcept {
            return components_count_ == 0u;
        }

        void clear();

        /// Entity must belong to default(empty) archetype
        ArchetypeEntityIndex insert(Entity entity, const ComponentIdMask& skip_constructor = {});

        void externalMove(Entity entity, Archetype& prev, ArchetypeEntityIndex prev_index,
                          const ComponentIdMask& skip_constructor);
        void internalMove(ArchetypeEntityIndex from, ArchetypeEntityIndex to);
        /**
         * removes entity from archetype, calls destructor for each trivially destructible component
         * moves last entity at index.
         * returns new entity at index.
         */
        void remove(Entity entity, ArchetypeEntityIndex index);
        void callDestructor(const ElementView& view);

        World& world_;
        const ComponentIdMask mask_;
        const SharedComponentsInfo shared_components_info_;
        ArchetypeOperationHelper operation_helper_;
        std::unique_ptr<BaseComponentDataStorage> data_storage_;
        ArrayWrapper<Entity, ArchetypeEntityIndex, true> entities_;
        const uint32_t components_count_;
        const uint32_t chunk_size_ = 1024u;
        // TODO: change the way data stored and make in ArrayWrapper
        std::vector<WorldVersion, Allocator<WorldVersion> > chunk_versions_; // per chunk component version
        mustache::ArrayWrapper<WorldVersion, ComponentIndex, true> global_versions_; // global component version

//        mustache::ArrayWrapper<SharedComponentIndex, std::shared_ptr<void>, true> shared_components_;
        const ArchetypeIndex id_;
    };

    template<FunctionSafety _Safety>
    Entity* ElementView::getEntity() const {
        if constexpr (isSafe(_Safety)) {
            if (!isValid()) {
                return nullptr;
            }
        }
        const auto gi = globalIndex();
        if (archetype_->entities_.size() <= gi.toInt()) {
            return nullptr;
        }
        // TODO: remove const_cast
        return const_cast<Entity*>(archetype_->entities_.data()) + gi.toInt();
    }


    template<FunctionSafety _Safety>
    MUSTACHE_INLINE void* ElementView::getSharedComponent(SharedComponentIndex index) const noexcept {
        return archetype_->template getSharedComponent<_Safety>(index);
    }

}
