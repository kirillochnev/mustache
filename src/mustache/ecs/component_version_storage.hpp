#pragma once

#include <mustache/utils/profiler.hpp>
#include <mustache/utils/array_wrapper.hpp>

#include <mustache/ecs/id_deff.hpp>
#include <mustache/ecs/component_mask.hpp>

#include <limits>

namespace mustache {
    class MemoryManager;

    struct MaskAndVersion {
        WorldVersion version;
        std::vector<ComponentIndex> mask;
    };

    class VersionStorage : public Uncopiable {
    public:

        VersionStorage(MemoryManager& memory_manager,
                       uint32_t num_components,
                       uint32_t chunk_size,
                       const ComponentIndexMask& enabled_for):
                chunk_size_{chunk_size},
                chunk_versions_{memory_manager},
                global_versions_{memory_manager},
                enabled_mask_{enabled_for} {
            MUSTACHE_PROFILER_BLOCK_LVL_0(__FUNCTION__);
            global_versions_.resize(num_components);
        }

        [[nodiscard]] bool enabled() const noexcept {
            return !enabled_mask_.isEmpty();
        }

        void emplace(WorldVersion version, ArchetypeEntityIndex index) noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_2(__FUNCTION__);
            if (!enabled()) {
                return;
            }
            const auto chunk = chunkAt(index);
            if (chunk.template toInt<size_t>() * numComponents() <= chunk_versions_.size()) {
                chunk_versions_.resize(chunk.next().template toInt<size_t>() * numComponents());
            }

            setVersion(version, chunk);
        }

        void setVersion(WorldVersion version, ChunkIndex chunk) noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            if (!enabled()) {
                return;
            }
            const auto begin = numComponents() * chunk.toInt();
            for (uint32_t i = 0; i < numComponents(); ++i) {
                global_versions_[ComponentIndex::make(i)] = version;
                chunk_versions_[begin + i] = version;
            }
        }

        void setVersion(WorldVersion version, ComponentIndex component_index) noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            global_versions_[component_index] = version;
        }

        void setVersion(WorldVersion version, ChunkIndex chunk, ComponentIndex component) noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            if (!enabled_mask_.has(component)) {
                return;
            }
            const auto update_index = numComponents() * chunk.toInt() + component.toInt();
            chunk_versions_[update_index] = version;
            setVersion(version, component);
        }

        void setVersion(WorldVersion version, ArchetypeEntityIndex index, ComponentIndex component) noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            if (!enabled_mask_.has(component)) {
                return;
            }
            const auto chunk = chunkAt(index);
            const auto update_index = numComponents() * chunk.toInt() + component.toInt();
            chunk_versions_[update_index] = version;
            setVersion(version, component);
        }

        [[nodiscard]] WorldVersion getVersion(ComponentIndex component) const noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            return global_versions_[component];
        }

        [[nodiscard]] WorldVersion getVersion(ChunkIndex chunk, ComponentIndex component) const noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            if (!enabled_mask_.has(component)) {
                return WorldVersion::null();
            }
            const WorldVersion* chunk_version = chunk_versions_.data() + numComponents() * chunk.toInt();
            return chunk_version[component.toInt()];
        }

        [[nodiscard]] bool checkAndSet(const MaskAndVersion& check, const MaskAndVersion& set) noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            if (!enabled()) {
                return true;
            }

            bool need_update = check.version.isNull() || check.mask.empty();

            for (auto index : check.mask) {
                if (global_versions_[index] > check.version) {
                    need_update = true;
                    break;
                }
            }

            if (need_update) {
                for (auto index : set.mask) {
                    global_versions_[index] = set.version;
                }
            }
            return need_update;
        }

        [[nodiscard]] bool checkAndSet(const MaskAndVersion& check, const MaskAndVersion& set, ChunkIndex chunk) noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            if (!enabled()) {
                return true;
            }

            const auto first_index = numComponents() * chunk.toInt();
            auto versions = chunk_versions_.data() + first_index;
            bool result = check.version.isNull() || check.mask.empty();
            if (!result) {
                for (auto component_index : check.mask) {
                    if (versions[component_index.toInt()] > check.version) {
                        result = true;
                        break;
                    }
                }
            }

            if (result) {
                for (auto component_index : set.mask) {
                    versions[component_index.toInt()] = set.version;
                }
            }
            return result;
        }

        [[nodiscard]] uint32_t numComponents() const noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            return static_cast<uint32_t >(global_versions_.size());
        }

        [[nodiscard]] ChunkIndex chunkAt(ArchetypeEntityIndex index) const noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            if (!enabled()) {
                return ChunkIndex::make(0);
            }
            return ChunkIndex::make(index.toInt() / chunk_size_);
        }

        [[nodiscard]] uint32_t chunkSize() const noexcept {
            MUSTACHE_PROFILER_BLOCK_LVL_3(__FUNCTION__);
            if (!enabled()) {
                return std::numeric_limits<uint32_t >::max();
            }
            return chunk_size_;
        }
        [[nodiscard]] const auto& enabledMask() const noexcept {
            return enabled_mask_;
        }
        void setEnabledMask(const ComponentIndexMask& mask) noexcept {
            enabled_mask_ = mask;
        }
    protected:
        friend class Archetype;
        uint32_t chunk_size_;
        mustache::vector<WorldVersion, Allocator<WorldVersion> > chunk_versions_; // per chunk component version
        mustache::ArrayWrapper<WorldVersion, ComponentIndex, true> global_versions_; // global component version
        ComponentIndexMask enabled_mask_;
    };

    class FakeVersionStorage {
    public:
        constexpr FakeVersionStorage(MemoryManager&, uint32_t, uint32_t) {}

        [[nodiscard]] constexpr bool enabled() const noexcept {
            return false;
        }

        constexpr void emplace(WorldVersion, ArchetypeEntityIndex) noexcept {}

        constexpr void setVersion(WorldVersion, ChunkIndex) noexcept {}

        constexpr void setVersion(WorldVersion, ComponentIndex) noexcept {}

        constexpr void setVersion(WorldVersion, ChunkIndex, ComponentIndex) noexcept {}

        constexpr void setVersion(WorldVersion, ArchetypeEntityIndex, ComponentIndex) noexcept {}

        [[nodiscard]] constexpr WorldVersion getVersion(ComponentIndex) const noexcept {
            return mustache::WorldVersion::null();
        }

        [[nodiscard]] constexpr WorldVersion getVersion(ChunkIndex, ComponentIndex) const noexcept {
            return mustache::WorldVersion::null();
        }

        [[nodiscard]] constexpr bool checkAndSet(const MaskAndVersion&, const MaskAndVersion&) noexcept {
            return true;
        }

        [[nodiscard]] constexpr bool checkAndSet(const MaskAndVersion&, const MaskAndVersion&, ChunkIndex) noexcept {
            return true;
        }

        [[nodiscard]] constexpr ChunkIndex chunkAt(ArchetypeEntityIndex) const noexcept {
            return ChunkIndex::make(0);
        }

        [[nodiscard]] constexpr uint32_t chunkSize() const noexcept {
            return std::numeric_limits<uint32_t>::max();
        }
    };
}
