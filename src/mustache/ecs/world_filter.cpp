#include "job.hpp"
#include "world_filter.hpp"

using namespace mustache;

void WorldFilterResult::ArchetypeFilterResult::addBlock(const EntityBlock& block) noexcept {
    const auto block_size = block.end.toInt() - block.begin.toInt();
    if (block_size > 0) {
        entities_count += block_size;
        blocks.push_back(block);
    }
}

void WorldFilterResult::filterArchetype(Archetype& archetype, const ArchetypeFilterParam& check,
                                        const ArchetypeFilterParam& set) {
    ArchetypeFilterResult item;
    item.archetype = &archetype;
    item.entities_count = 0u;

    const auto last_index = archetype.lastChunkIndex();
    bool is_prev_match = false;
    EntityBlock block{ArchetypeEntityIndex::make(0), ArchetypeEntityIndex::make(0)};
    const auto chunk_size = archetype.chunkCapacity().toInt();
    for (auto chunk_index = ChunkIndex::make(0); chunk_index <= last_index; ++chunk_index) {
        const bool is_match = archetype.updateChunkComponentVersions<FunctionSafety::kUnsafe>(check, set, chunk_index);
        if (is_match) {
            if (!is_prev_match) {
                block.begin = ArchetypeEntityIndex::make(chunk_index.toInt() * chunk_size);
            }
            block.end = ArchetypeEntityIndex::make(chunk_index.next().toInt() * chunk_size);
        } else {
            if (is_prev_match) {
                item.addBlock(block);
            }
        }
        is_prev_match = is_match;
    }
    if (is_prev_match) {
        const auto end_of_archetype = ArchetypeEntityIndex::make(archetype.size());
        block.end = std::min(end_of_archetype, block.end);
        item.addBlock(block);
    }
    if (item.entities_count > 0) {
        filtered_archetypes.push_back(item);
        total_entity_count += item.entities_count;
    }
}

void WorldFilterResult::apply(World& world, const WorldFilterParam& check, const WorldFilterParam& set) {
    filtered_archetypes.clear();
    total_entity_count = 0;

    auto& entities = world.entities();
    const auto num_archetypes = entities.getArchetypesCount();
    filtered_archetypes.reserve(num_archetypes);


    ArchetypeFilterParam archetype_check;
    archetype_check.version = check.version;
    for (ArchetypeIndex index = ArchetypeIndex::make(0); index < ArchetypeIndex::make(num_archetypes); ++index) {
        auto& arch = entities.getArchetype(index);
        if (arch.size() > 0u && arch.isMatch(mask_)) {
            archetype_check.components = arch.makeComponentMask(check.mask).items();
            ArchetypeFilterParam archetype_set;
            archetype_set.version = set.version;
            archetype_set.components = arch.makeComponentMask(set.mask).items();
            if (arch.updateGlobalComponentVersion(archetype_check, archetype_set)) {
                filterArchetype(arch, archetype_check, archetype_set);
            }
        }
    }
}
