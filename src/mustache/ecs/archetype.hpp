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

        /*explicit ElementView(const Super& view) noexcept :
                Super{view} {

        }*/
        ElementView(const Super& view, const Archetype& archetype):
            Super{view},
            archetype_{&archetype} {

        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        MUSTACHE_INLINE Entity* getEntity() const;

        const Archetype* archetype_ = nullptr;
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
            return entities_[index.toInt()];
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

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        [[nodiscard]] WorldVersion getWorldVersionComponentUpdate(ComponentId id,
                ArchetypeEntityIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (!hasComponent(id)) {
                    return WorldVersion::null();
                }
            }
            return data_storage_.getVersion(getComponentIndex(id), ComponentStorageIndex::fromArchetypeIndex(index));
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
        void* getComponent(ComponentIndex component_index, ArchetypeEntityIndex index) const noexcept {
            return data_storage_.getData<_Safety>(component_index, ComponentStorageIndex::fromArchetypeIndex(index));
        }

        [[nodiscard]] ElementView getElementView(ArchetypeEntityIndex index) const noexcept {
            return ElementView {
                data_storage_.getElementView(ComponentStorageIndex::fromArchetypeIndex(index)),
                *this
            };
        }
    private:
        friend ElementView;
        friend EntityManager;

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
        void callDestructor(const ArchetypeComponentDataStorage::ElementView& view);

        World& world_;
        ComponentIdMask mask_;
        ArchetypeOperationHelper operation_helper_;
        ArchetypeComponentDataStorage data_storage_;
        std::vector<Entity> entities_;

        ArchetypeIndex id_;
    };

    template<FunctionSafety _Safety>
    Entity* ElementView::getEntity() const {
        auto storage_res = _getEntity<_Safety>();
        const auto gi = globalIndex();
        const auto foo = [this, gi]()-> Entity* {
            if constexpr (isSafe(_Safety)) {
                if (!isValid()) {
                    return nullptr;
                }
            }
            if (archetype_->entities_.size() <= gi.toInt()) {
                return nullptr;
            }
            // TODO: remove const_cast
            return const_cast<Entity*>(archetype_->entities_.data()) + gi.toInt();
        };
        const auto to_str = [](Entity* e)-> std::string {
            if (e == nullptr) {
                return "nullptr";
            }
            return std::to_string(e->value);
        };

        auto arch_res = foo();
        if ((storage_res == nullptr && arch_res == nullptr) || *arch_res == *storage_res) {
            return arch_res;
        }
        throw std::runtime_error("Invalid entity at: " + std::to_string(gi.toInt()) + " " +
                                         to_str(arch_res) + " vs " + to_str(storage_res));
    }
}
