#pragma once

#include <vector>
#include <cstdint>
#include <mustache/ecs/id_deff.hpp>

namespace mustache {

    struct Entity {

        constexpr Entity() = default;

        constexpr Entity(EntityId id, EntityVersion version, WorldId world_id) noexcept {
            reset(id, version, world_id);
        }

        [[nodiscard]] static constexpr Entity makeFromValue(uint64_t value) noexcept {
            Entity result;
            result.value = value;
            return result;
        }

        [[nodiscard]] constexpr bool isNull() const noexcept {
            return value == null;
        }

        [[nodiscard]] constexpr WorldId worldId() const noexcept {
            return WorldId::make(shiftedWorldId() >> world_id_shift);
        }

        [[nodiscard]] constexpr EntityId id() const noexcept {
            return EntityId::make(value & id_mask);
        }

        [[nodiscard]] constexpr EntityVersion version() const noexcept {
            return EntityVersion::make(shiftedVersion() >> version_shift);
        }

        void constexpr setVersion(const EntityVersion& version) noexcept {
            reset(id(), version, worldId());
        }

        constexpr void reset(EntityId id, EntityVersion version, WorldId world_id) noexcept {
            value = id.toInt<uint64_t>() |
                    (version.toInt<uint64_t>() << version_shift) |
                    (world_id.toInt<uint64_t>() << world_id_shift);
        }
        constexpr void reset(EntityId id, EntityVersion version) noexcept {
            reset(id, version, worldId());
        }
        constexpr void reset(EntityId id) noexcept {
            reset(id, version(), worldId());
        }
        constexpr void reset() noexcept {
            value = null;
        }

        [[nodiscard]] constexpr bool operator<(const Entity& rhs) const noexcept {
            return value < rhs.value;
        }

        [[nodiscard]] constexpr Entity makeEntityWithNextVersion() const noexcept {
            constexpr uint64_t version_diff = 1uLL << version_shift;
            return Entity{value + version_diff};
        }

        void incrementVersion() noexcept {
            constexpr uint64_t version_diff = 1uLL << version_shift;
            value += version_diff;
        }

        uint64_t value{static_cast<uint64_t >(-1)};

        [[nodiscard]] constexpr bool operator==(const Entity& rhs) const noexcept {
            return value == rhs.value;
        }
        [[nodiscard]] constexpr bool operator!=(const Entity& rhs) const noexcept {
            return value != rhs.value;
        }
    private:
        constexpr Entity(uint64_t v) noexcept :
            value{v} {

        }
        /*
         * id : entity_id_bits
         * world : version_bits
         * version : world_id_bits
         *
         * */

        enum : uint64_t {
            null = static_cast<uint64_t>(-1),
        };
        static constexpr uint64_t entity_id_bits = 30ull;
        static constexpr uint64_t version_bits = 24ull;
        static constexpr uint64_t world_id_bits = 10ull;
        static_assert(entity_id_bits + version_bits + world_id_bits == 64ull, "Invalid entity struct");

        static constexpr uint64_t max_entity_per_world = (1lu << entity_id_bits);
        static constexpr uint64_t max_entity_version = (1lu << version_bits);
        static constexpr uint64_t max_world_count = (1lu << world_id_bits);
        static constexpr uint64_t id_mask = max_entity_per_world - 1;
        static constexpr uint64_t version_mask = max_entity_version - 1;
        static constexpr uint64_t world_id_mask = max_world_count - 1;

        static constexpr uint64_t entity_id_shift = 0;
        static constexpr uint64_t world_id_shift = entity_id_shift + entity_id_bits;
        static constexpr uint64_t version_shift = world_id_shift + world_id_bits;
        static constexpr uint64_t shifted_world_id_mask = world_id_mask << world_id_shift;
        static constexpr uint64_t shifted_version_mask = version_mask << version_shift;

        [[nodiscard]] constexpr uint64_t shiftedWorldId() const noexcept {
            return value & shifted_world_id_mask;
        }
        [[nodiscard]] constexpr uint64_t shiftedVersion() const noexcept {
            return value & shifted_version_mask;
        }
    };
}
