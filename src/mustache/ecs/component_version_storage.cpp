#include "component_version_storage.hpp"

using namespace mustache;

VersionStorage::VersionStorage(MemoryManager& memory_manager, uint32_t num_components, uint32_t chunk_size):
        chunk_size_{chunk_size},
        chunk_versions_{memory_manager},
        global_versions_{memory_manager} {
    global_versions_.resize(num_components);
}

void VersionStorage::emplace(WorldVersion version, ArchetypeEntityIndex index) noexcept {
    const auto chunk = chunkAt(index);
    if (chunk.toInt() * numComponents() <= chunk_versions_.size()) {
        chunk_versions_.resize(chunk.next().toInt() * numComponents());
    }

    setVersion(version, chunk);
}

void VersionStorage::setVersion(WorldVersion version, ChunkIndex chunk) noexcept {
    WorldVersion* chunk_version = chunk_versions_.data() + numComponents() * chunk.toInt();
    for (uint32_t i = 0; i < numComponents(); ++i) {
        chunk_versions_[i] = version;
        chunk_version[i] = version;
    }
}

void VersionStorage::setVersion(WorldVersion version, ComponentIndex component_index) noexcept {
    global_versions_[component_index] = version;
}

void VersionStorage::setVersion(WorldVersion version, ChunkIndex chunk, ComponentIndex component) noexcept {
    WorldVersion* chunk_version = chunk_versions_.data() + numComponents() * chunk.toInt();
    chunk_version[component.toInt()] = version;
    setVersion(version, chunk);
}

WorldVersion VersionStorage::getVersion(ComponentIndex component) const noexcept {
    return global_versions_[component];
}

WorldVersion VersionStorage::getVersion(ChunkIndex chunk, ComponentIndex component) const noexcept {
    const WorldVersion* chunk_version = chunk_versions_.data() + numComponents() * chunk.toInt();
    return chunk_version[component.toInt()];
}

bool VersionStorage::checkAndSet(const MaskAndVersion& check, const MaskAndVersion& set) noexcept {
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

bool VersionStorage::checkAndSet(const MaskAndVersion& check, const MaskAndVersion& set, ChunkIndex chunk) noexcept {
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

uint32_t VersionStorage::numComponents() const noexcept {
    return static_cast<uint32_t >(global_versions_.size());
}

ChunkIndex VersionStorage::chunkAt(ArchetypeEntityIndex index) const noexcept {
    return ChunkIndex::make(index.toInt() / chunk_size_);
}

uint32_t VersionStorage::chunkSize() const noexcept {
    return chunk_size_;
}
