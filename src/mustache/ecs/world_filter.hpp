#pragma once

#include <mustache/ecs/world.hpp>

namespace mustache {

    struct WorldFilterParam {
        ComponentIdMask mask;
        WorldVersion version;
    };

    /**
     * Stores result of archetype filtering
     */
    struct WorldFilterResult {
        struct EntityBlock {
            ArchetypeEntityIndex begin;
            ArchetypeEntityIndex end;
        };
        struct ArchetypeFilterResult {
            Archetype* archetype {nullptr};
            uint32_t entities_count {0};
            void addBlock(const EntityBlock& block) noexcept;
            std::vector<EntityBlock> blocks;
        };
        void apply(World& world, const WorldFilterParam& check, const WorldFilterParam& set);
        void filterArchetype(Archetype& archetype, const ArchetypeFilterParam& check, const ArchetypeFilterParam& set);
        std::vector<ArchetypeFilterResult> filtered_archetypes;
        ComponentIdMask mask_;
        uint32_t total_entity_count{0u};
    };

}
