#pragma once

#include <mustache/utils/index_like.hpp>
#include <mustache/utils/default_settings.hpp>
#include <cstdint>
#include <cstddef>
#include <iostream>

namespace mustache {
    struct MUSTACHE_EXPORT EntityId : public IndexLike<uint32_t, EntityId>{};
    struct MUSTACHE_EXPORT EntityVersion : public IndexLike<uint32_t, EntityVersion>{};
    struct MUSTACHE_EXPORT WorldId : public IndexLike<uint32_t, WorldId>{};
    struct MUSTACHE_EXPORT WorldVersion : public IndexLike<uint32_t, WorldVersion>{};

    struct MUSTACHE_EXPORT ComponentId : public IndexLike<uint32_t, ComponentId>{};
    struct MUSTACHE_EXPORT SharedComponentId : public IndexLike<uint32_t, SharedComponentId>{};

    /// Index archetype in entity manager
    struct MUSTACHE_EXPORT ArchetypeIndex : public IndexLike<uint32_t , ArchetypeIndex> {};

    /// Index archetype in task
    struct MUSTACHE_EXPORT TaskArchetypeIndex : public IndexLike<uint32_t , TaskArchetypeIndex> {};

    /// index of entity in archetype
    struct MUSTACHE_EXPORT ArchetypeEntityIndex : public IndexLike<uint32_t, ArchetypeEntityIndex> {};

    /// index of item in chunk
    struct MUSTACHE_EXPORT ChunkItemIndex : public IndexLike<uint32_t, ChunkItemIndex> {};

    struct MUSTACHE_EXPORT ChunkCapacity : public mustache::IndexLike<uint32_t, ChunkCapacity, 0u> {
        [[nodiscard]] bool isIndexValid(ChunkItemIndex index) const noexcept {
            return index.toInt() < value_;
        }
    };

    /// Location of data in component data storage
    struct MUSTACHE_EXPORT DataLocation : public mustache::IndexLike<uint32_t, DataLocation> {};

    /// index of chunk in archetype
    struct MUSTACHE_EXPORT ChunkIndex : public IndexLike<uint32_t, ChunkIndex> {};

    /// index of element in component data storage
    struct MUSTACHE_EXPORT ComponentStorageIndex : public mustache::IndexLike<uint32_t, ComponentStorageIndex> {
        [[nodiscard]] static ComponentStorageIndex fromArchetypeIndex(ArchetypeEntityIndex index) noexcept {
            return make(index.toInt());
        }
        [[nodiscard]] constexpr ChunkIndex operator/(ChunkCapacity capacity) const noexcept {
            if (capacity.isNull()) {
                return ChunkIndex::null();
            }
            return ChunkIndex::make(value_ / capacity.toInt());
        }
        [[nodiscard]] constexpr ChunkItemIndex operator%(ChunkCapacity capacity) const noexcept {
            if (capacity.isNull()) {
                return ChunkItemIndex::null();
            }
            return ChunkItemIndex::make(value_ % capacity.toInt());
        }
        [[nodiscard]] constexpr ArchetypeEntityIndex toArchetypeIndex() const noexcept {
            return ArchetypeEntityIndex::make(value_);
        }
    };

    /// index(position) of component in archetype
    struct MUSTACHE_EXPORT ComponentIndex : public IndexLike<uint32_t, ComponentIndex>{};

    struct MUSTACHE_EXPORT SharedComponentIndex : public IndexLike<uint32_t, SharedComponentIndex>{};

    // size of component array for Job::forEachArray function, this allows to use int-types as components
    struct MUSTACHE_EXPORT ComponentArraySize : public IndexLike<uint32_t, ComponentArraySize>{};

    // offset of component data in the chunk
    struct MUSTACHE_EXPORT ComponentOffset : public IndexLike<uint32_t, ComponentOffset> {
        template <typename T = std::byte>
        constexpr ComponentOffset add(size_t i) const noexcept {
            return ComponentOffset::make(value_ + static_cast<ValueType>(i * sizeof(T)));
        }
        [[nodiscard]] constexpr std::byte* apply(std::byte* ptr) const noexcept {
            return ptr + value_;
        }
        [[nodiscard]] constexpr const std::byte* apply(const std::byte* ptr) const noexcept {
            return ptr + value_;
        }

        [[nodiscard]] static constexpr ComponentOffset makeAligned(ComponentOffset offset, uint32_t align) noexcept {
            return ComponentOffset::make((offset.toInt() - 1u + align) / align * align);
        }

        [[nodiscard]] constexpr ComponentOffset alignAs(uint32_t align) const noexcept {
            return ComponentOffset::make((toInt() - 1u + align) / align * align);
        }
    };
}
