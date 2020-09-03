#pragma once

#include <mustache/utils/index_like.hpp>
#include <mustache/utils/default_settings.hpp>
#include <cstdint>
#include <cstddef>

namespace mustache {
    struct EntityId : public IndexLike<uint32_t, EntityId>{};
    struct EntityVersion : public IndexLike<uint32_t, EntityVersion>{};
    struct WorldId : public IndexLike<uint32_t, WorldId>{};

    struct ComponentId : public IndexLike<uint32_t, ComponentId>{};

    struct PerEntityJobTaskId : public IndexLike<uint32_t , PerEntityJobTaskId> {};
    struct PerEntityJobEntityIndexInTask : public IndexLike<uint32_t , PerEntityJobEntityIndexInTask> {};

    /// Index archetype in entity manager
    struct ArchetypeIndex : public IndexLike<uint32_t , ArchetypeIndex> {};

    /// Index archetype in task
    struct TaskArchetypeIndex : public IndexLike<uint32_t , TaskArchetypeIndex> {};

    /// index of entity in archetype
    struct ArchetypeEntityIndex : public IndexLike<uint32_t, ArchetypeEntityIndex> {};

    /// index of entity in archetype
    struct ChunkIndex : public IndexLike<uint32_t, ChunkIndex> {};

    /// index of entity in chunk
    struct ChunkEntityIndex : public IndexLike<uint32_t, ChunkEntityIndex> {};

    /// index(position) of component in archetype
    struct ComponentIndex : public IndexLike<uint32_t, ComponentIndex>{
        [[nodiscard]] MUSTACHE_INLINE constexpr ComponentIndex nextIndex() const noexcept {
            return ComponentIndex::make(value_ + 1);
        }
    };

    struct ComponentOffset : public IndexLike<uint32_t, ComponentOffset> {
        template <typename T = std::byte>
        MUSTACHE_INLINE constexpr ComponentOffset add(size_t i) const noexcept {
            return ComponentOffset::make(value_ + static_cast<ValueType>(i * sizeof(T)));
        }
        [[nodiscard]] MUSTACHE_INLINE constexpr std::byte* apply(std::byte* ptr) const noexcept {
            return ptr + value_;
        }
        [[nodiscard]] MUSTACHE_INLINE constexpr const std::byte* apply(const std::byte* ptr) const noexcept {
            return ptr + value_;
        }
    };
}