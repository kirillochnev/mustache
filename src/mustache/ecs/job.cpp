#include "job.hpp"
#include "world_filter.hpp"

#include <mustache/utils/benchmark.hpp>
using namespace mustache;

void DefaultWorldFilterResult::ArchetypeFilterResult::addBlock(const EntityBlock& block) noexcept {
    const auto block_size = block.end - block.begin;
    if (block_size > 0) {
        entities_count += block_size;
        blocks.push_back(block);
    }
}

void DefaultWorldFilterResult::filterArchetype(World& world, Archetype& archetype, const ComponentIdMask& write_mask,
        WorldVersion prev_version) {
    if (archetype.size() < 1) {
        return;
    }
    const auto cur_version = world.version();
    ArchetypeFilterResult item;
    item.archetype = &archetype;
    item.entities_count = 0u;

    ComponentIndexMask mask = archetype.makeComponentMask(write_mask);
    const auto last_index = archetype.lastChunkIndex();
    const auto component_to_update = mask.items();
    const std::vector<ComponentIndex> components_to_check;
    bool is_prev_match = false;
    EntityBlock block{0, 0};
    const auto chunk_size = archetype.chunkCapacity().toInt();
    for (auto chunk_index = ChunkIndex::make(0); chunk_index <= last_index; ++chunk_index) {
        const bool is_match = archetype.updateComponentVersions<FunctionSafety::kUnsafe>(components_to_check,
                prev_version, component_to_update, cur_version, chunk_index);
        if (is_match) {
            if (!is_prev_match) {
                block.begin = chunk_index.toInt() * chunk_size;
            }
            block.end = chunk_index.next().toInt() * chunk_size;
        } else {
            if (is_prev_match) {
                item.addBlock(block);
            }
        }
        is_prev_match = is_match;
    }
    if (is_prev_match) {
        block.end = std::min(archetype.size(), block.end);
        item.addBlock(block);
    }
    if (item.entities_count > 0) {
        filtered_archetypes.push_back(item);
        total_entity_count += item.entities_count;
    }
}

void DefaultWorldFilterResult::apply(World& world, const ComponentIdMask& write_mask, WorldVersion prev_version) {
    filtered_archetypes.clear();
    total_entity_count = 0;

    auto& entities = world.entities();
    const auto num_archetypes = entities.getArchetypesCount();
    filtered_archetypes.reserve(num_archetypes);

//    Benchmark benchmark;
//    benchmark.add([&] {
        for (ArchetypeIndex index = ArchetypeIndex::make(0); index < ArchetypeIndex::make(num_archetypes); ++index) {
            auto& arch = entities.getArchetype(index);
            if (arch.size() > 0u && arch.isMatch(mask_)) {
                filterArchetype(world, arch, write_mask, prev_version);
            }
        }
//    }, 100);
//    benchmark.show();
}
