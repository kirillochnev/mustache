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
                const auto version = entities_[id].version();
                next_slot_ = entities_[id].id().toInt();
                entity.reset(EntityId::make(id), version);
                entities_[id] = entity;
                locations_[id].archetype = archetype.id();
                locations_[id].index = archetype.insert(entity);
                --empty_slots_;
            }
            return entity;
        }

        void clear();
        void destroy(Entity entity);

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
        std::vector<std::unique_ptr<Archetype> > archetypes_;

        uint64_t next_slot_{0};
        uint32_t empty_slots_{0};
        std::vector<Entity> entities_;
        std::vector<EntityLocationInWorld> locations_;
    };

}