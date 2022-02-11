#pragma once

#include <mustache/utils/array_wrapper.hpp>

#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/component_mask.hpp>

namespace mustache {

    class Archetype;

    struct MUSTACHE_EXPORT WorldFilterParam {
        WorldFilterParam() = default;
        WorldFilterParam(const ComponentIdMask& m, const WorldVersion v):
            mask{m},
            version{v} {

        }
        ComponentIdMask mask;
        WorldVersion version;
    };
    struct MUSTACHE_EXPORT FilterCheckParam : public WorldFilterParam {
        using WorldFilterParam::WorldFilterParam;
    };
    struct MUSTACHE_EXPORT FilterSetParam : public WorldFilterParam {
        using WorldFilterParam::WorldFilterParam;
    };

    /**
     * Stores result of archetype filtering
     */
    struct MUSTACHE_EXPORT WorldFilterResult {
        struct EntityBlock {
            ArchetypeEntityIndex begin;
            ArchetypeEntityIndex end;
        };

        struct BlockIndex : public IndexLike<uint32_t, BlockIndex>{};

        struct ArchetypeFilterResult {
            Archetype* archetype {nullptr};
            uint32_t entities_count {0};
            void addBlock(const EntityBlock& block) noexcept;
            ArrayWrapper<EntityBlock, BlockIndex, false> blocks; // TODO: use memory manager
        };

        void clear() noexcept;

        std::vector<ArchetypeFilterResult> filtered_archetypes;
        ComponentIdMask mask;
        SharedComponentIdMask shared_component_mask;
        uint32_t total_entity_count{0u};
    };
}
