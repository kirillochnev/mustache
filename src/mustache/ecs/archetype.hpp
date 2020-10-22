#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache/ecs/archetype_operation_helper.hpp>
#include <mustache/ecs/component_data_storage.hpp>
#include <mustache/ecs/new_component_data_storage.hpp>
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

    using ArchetypeComponentDataStorage = ComponentDataStorage;

    class Archetype;

    struct ElementView : public ArchetypeComponentDataStorage::ElementView {
        using Super = ArchetypeComponentDataStorage::ElementView;
        using Super::Super;

        ElementView(const Super& view, const Archetype& archetype):
            Super{view},
            archetype_{&archetype} {

        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE Entity* getEntity() const;

        const Archetype* archetype_ = nullptr;
    };

    struct ArchetypeFilterParam {
        std::vector<ComponentIndex> components;
        WorldVersion version;
    };

    /**
     * Stores Entities with same component set
     * NOTE: It is has no information about entity manager, so Archetype's methods don't effects entity location.
     */
    class Archetype : public Uncopiable {
    public:
        Archetype(World& world, ArchetypeIndex id, const ComponentIdMask& mask);
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
            return data_storage_.size();
        }
        [[nodiscard]] uint32_t capacity() const noexcept {
            return data_storage_.capacity();
        }
        [[nodiscard]] ArchetypeIndex id() const noexcept {
            return id_;
        }

        [[nodiscard]] uint32_t chunkCount() const noexcept {
            return entities_.size() / chunk_size_;
        }

        [[nodiscard]] ChunkCapacity chunkCapacity() const noexcept {
            return ChunkCapacity::make(chunk_size_);
        }

        bool isMatch(const ComponentIdMask& mask) const noexcept {
            return mask_.isMatch(mask);
        }

        bool hasComponent(ComponentId component_id) const noexcept {
            return mask_.has(component_id);
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
            return data_storage_.getData<_Safety>(operation_helper_.componentIndex(component_id),
                    ComponentStorageIndex::fromArchetypeIndex(index));
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        const void* getConstComponent(ComponentIndex component_index, ArchetypeEntityIndex index) const noexcept {
            return data_storage_.getData<_Safety>(component_index, ComponentStorageIndex::fromArchetypeIndex(index));
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void* getComponent(ComponentIndex component_index, ArchetypeEntityIndex index, WorldVersion version) noexcept {
            auto res = data_storage_.getData<_Safety>(component_index, ComponentStorageIndex::fromArchetypeIndex(index));
            if (res != nullptr) {
                const auto chunk = index.toInt() / chunk_size_;
                auto versions = versions_.data() + components_count_ * chunk;
                versions[component_index.toInt()] = version;
            }

            return res;
        }

        [[nodiscard]] ElementView getElementView(ArchetypeEntityIndex index) const noexcept {
            return ElementView {
                data_storage_.getElementView(ComponentStorageIndex::fromArchetypeIndex(index)),
                *this
            };
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] WorldVersion getComponentVersion(ChunkIndex index, ComponentIndex component_index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (components_count_ <= component_index.toInt() ||
                    versions_.size() / components_count_ <= index.toInt()) {
                    return WorldVersion::null();
                }
            }
            return versions_[versionIndex(index, component_index)];
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
                result = ChunkIndex::make(entities_count / chunk_size_);
            }
            return result;
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        bool updateComponentVersions(const ArchetypeFilterParam& check,
                const ArchetypeFilterParam& set, ChunkIndex chunk) noexcept {
            const auto first_index = components_count_ * chunk.toInt();
            if constexpr (isSafe(_Safety)) {
                const auto last_index = first_index + components_count_;
                if (!chunk.isValid() || !set.version.isValid() || versions_.size() <= last_index) {
                    return false;
                }
            }
            auto versions = versions_.data() + first_index;
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

    private:
        friend ElementView;
        friend EntityManager;

        // NOTE: can resize versions_
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        void updateAllVersions(ArchetypeEntityIndex index, WorldVersion world_version) noexcept {
            if constexpr (isSafe(_Safety)) {
                if (!entities_.has(index) || !index.isValid() || !world_version.isValid()) {
                    return;
                }
            }
            const auto first_index = components_count_ * index.toInt() / chunk_size_;
            const auto last_index = first_index + components_count_;

            if (versions_.size() <= last_index) {
                versions_.resize(last_index + 1);
            }
            for (uint32_t i = first_index; i < last_index; ++i) {
                versions_[i] = world_version;
            }
        }

        MUSTACHE_INLINE uint32_t versionIndex(ChunkIndex chunk_index, ComponentIndex component_index) const noexcept {
            return chunk_index.toInt() * components_count_ + component_index.toInt();
        }

        template<typename _F>
        void forEachEntity(_F&& callback) {
            if (data_storage_.isEmpty()) {
                return;
            }
            auto view = getElementView(ArchetypeEntityIndex::make(0));
            while (view.isValid()) {
                callback(*view.getEntity<FunctionSafety::kUnsafe>());
                ++view;
            }
        }

        void clear();

        /// Entity must belong to default(empty) archetype
        ArchetypeEntityIndex insert(Entity entity, bool call_constructor = true);

        void externalMove(Entity entity, Archetype& prev, ArchetypeEntityIndex prev_index, bool init_missing = true);
        void internalMove(ArchetypeEntityIndex from, ArchetypeEntityIndex to);
        /**
         * removes entity from archetype, calls destructor for each trivially destructible component
         * moves last entity at index.
         * returns new entity at index.
         */
        void remove(Entity entity, ArchetypeEntityIndex index);
        void callDestructor(const ElementView& view);

        World& world_;
        ComponentIdMask mask_;
        ArchetypeOperationHelper operation_helper_;
        ArchetypeComponentDataStorage data_storage_;
        ArrayWrapper<std::vector<Entity>, ArchetypeEntityIndex> entities_;
        uint32_t components_count_;
        uint32_t chunk_size_ = 1024u;
        std::vector<WorldVersion> versions_;

        ArchetypeIndex id_;
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

}
