#pragma once

#include <mustache/utils/uncopiable.hpp>

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/archetype.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/utils/default_settings.hpp>
#include <memory>
#include <map>
#include <set>

namespace mustache {

    class World;
    class ComponentFactory;

    class EntityManager : public Uncopiable {
    public:
        explicit EntityManager(World& world);

        [[nodiscard]] Entity create();

        void update();

        // Checks if entity is valid for this World
        MUSTACHE_INLINE bool isEntityValid(Entity entity) const noexcept {
            const auto id = entity.id();  // it is ok to get id for no-valid entity, null id will be returned
            if (entity.isNull() || entity.worldId() != this_world_id_ ||
                id.toInt() >= entities_.size() || entities_[id.toInt()].version() != entity.version()) {
                return false;
            }
            return true;
        }

        template<typename _F>
        void forEachArchetype(_F&& func) noexcept (noexcept(func)) {
            for (auto& arh : archetypes_) {
                func(*arh);
            }
        }
        [[nodiscard]] MUSTACHE_INLINE Entity create(Archetype& archetype) {
            Entity entity;
            if(!empty_slots_) {
                entity = Entity {entities_.size()};
                entities_.push_back(entity);
                locations_.emplace_back(archetype.insert(entity), archetype.id());
            } else {
                const auto id = next_slot_;
                const auto version = entities_[id.toInt()].version();
                next_slot_ = entities_[id.toInt()].id();
                entity.reset(id, version);
                entities_[id.toInt()] = entity;
                locations_[id.toInt()].archetype = archetype.id();
                locations_[id.toInt()].index = archetype.insert(entity);
                --empty_slots_;
            }
            return entity;
        }

        void clear();

        void clearArchetype(Archetype& archetype);

        void destroy(Entity entity) {
            marked_for_delete_.insert(entity);
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        void destroyNow(Entity entity) {
            if constexpr (isSafe(_Safety)) {
                if (!isEntityValid(entity)) {
                    return;
                }
            }
            const auto id = entity.id();
            auto& location = locations_[id.toInt()];
            if (!location.archetype.isNull()) {
                if constexpr (isSafe(_Safety)) {
                    if (location.archetype.toInt() >= archetypes_.size()) {
                        throw std::runtime_error("Invalid archetype index");
                    }
                }
                auto& archetype = archetypes_[location.archetype.toInt()];
                auto moved_entity =  archetype->remove(location.index);
                if (!moved_entity.isNull()) {
                    locations_[moved_entity.id().toInt()] = location;
                }
                location.archetype = ArchetypeIndex::null();
            }
            entities_[id.toInt()].reset(empty_slots_ ? next_slot_ : id.next(), entity.version().next());
            next_slot_ = id;
            ++empty_slots_;
        }

        Archetype& getArchetype(const ComponentMask&);

        template<typename... ARGS>
        Archetype& getArchetype() {
            return getArchetype(ComponentFactory::makeMask<ARGS...>());
        }

        [[nodiscard]] MUSTACHE_INLINE size_t getArchetypesCount() const noexcept {
            return archetypes_.size();
        }

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        [[nodiscard]] MUSTACHE_INLINE Archetype& getArchetype(ArchetypeIndex index) noexcept (!isSafe(_Safety)) {
            if constexpr (isSafe(_Safety)) {
                if (index.toInt() >= archetypes_.size()) {
                    throw std::runtime_error("Index is out of bound");
                }
            }
            return *archetypes_[index.toInt()];
        }

        template<typename T>
        MUSTACHE_INLINE T* getComponent(Entity entity) const noexcept {
            static const auto component_id = ComponentFactory::registerComponent<T>();
            const auto& location = locations_[entity.id().toInt()];
            if (!location.archetype.isValid()) {
                return nullptr;
            }
            const auto& arch = archetypes_[location.archetype.toInt()];
            if (!arch->hasComponent(component_id)) {
                return nullptr;
            }
            auto ptr = arch->getComponentFromArchetype<FunctionSafety::kUnsafe>(location.index, component_id);
            return reinterpret_cast<T*>(ptr);
        }

        /*template<typename T, typename... _ARGS>
        MUSTACHE_INLINE T& assign(Entity e, _ARGS&&... args) {
            /// TODO: add entity version increment!
            static const auto component_id = ComponentFactory::registerComponent<T>();
            auto& location = locations_[e.id().toInt()];
            if(location.archetype.isNull()) {
                static const ComponentMask mask{component_id};
                auto& arch = getArchetype(mask);
                location.archetype = arch.id();
                if constexpr (sizeof...(_ARGS) < 1) {
                    location.index = arch.insert(e);
                    return *getComponentFromArchetypeUnsafe<T>(arch, location.index);
                }
                location.index = arch.addEntityWithoutInit(e);
                return *new(getComponentFromArchetypeUnsafe<T>(arch, location.index)) T{std::forward<_ARGS>(args)...};
            }
        }*/
    private:
        World& world_;
        std::map<ComponentMask , Archetype* > mask_to_arch_;
        EntityId next_slot_ = EntityId::make(0);

        uint32_t empty_slots_{0};
        std::vector<Entity> entities_;
        std::vector<EntityLocationInWorld> locations_;
        std::set<Entity> marked_for_delete_;
        WorldId this_world_id_;
        // TODO: replace shared pointed with some kind of unique_ptr but with deleter calling clearArchetype
        // NOTE: must be the last field(for correct default destructor).
        std::vector<std::shared_ptr<Archetype> > archetypes_;
    };
}