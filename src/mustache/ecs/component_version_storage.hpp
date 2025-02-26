#pragma once

#include <mustache/utils/array_wrapper.hpp>

#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/component_mask.hpp>

namespace mustache {
    class MemoryManager;

    struct MaskAndVersion {
        WorldVersion version;
        mustache::vector<ComponentIndex> mask;
    };

    class MUSTACHE_EXPORT VersionStorage : public Uncopiable {
    public:

        VersionStorage(MemoryManager& memory_manager, uint32_t num_components, uint32_t chunk_size);

        void emplace(WorldVersion version, ArchetypeEntityIndex index) noexcept;

        void setVersion(WorldVersion version, ChunkIndex chunk) noexcept;
        void setVersion(WorldVersion version, ComponentIndex component) noexcept;
        void setVersion(WorldVersion version, ChunkIndex chunk, ComponentIndex component) noexcept;

        [[nodiscard]] WorldVersion getVersion(ComponentIndex component) const noexcept;
        [[nodiscard]] WorldVersion getVersion(ChunkIndex chunk, ComponentIndex component) const noexcept;
        [[nodiscard]] WorldVersion getVersion(ArchetypeEntityIndex entity_index, ComponentIndex component) const noexcept {
            return getVersion(ChunkIndex::make(entity_index.toInt() / chunkSize()), component);
        }

        bool checkAndSet(const MaskAndVersion& check, const MaskAndVersion& set) noexcept;
        bool checkAndSet(const MaskAndVersion& check, const MaskAndVersion& set, ChunkIndex chunk) noexcept;

        [[nodiscard]] ChunkIndex chunkAt(ArchetypeEntityIndex) const noexcept;

        [[nodiscard]] uint32_t numComponents() const noexcept;
        [[nodiscard]] uint32_t chunkSize() const noexcept;
    protected:
        uint32_t chunk_size_;
        mustache::vector<WorldVersion, Allocator<WorldVersion> > chunk_versions_; // per chunk component version
        mustache::ArrayWrapper<WorldVersion, ComponentIndex, true> global_versions_; // global component version
    };
}
