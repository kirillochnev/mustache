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

void WorldFilterResult::clear() noexcept {
    total_entity_count = 0u;
    filtered_archetypes.clear();
}
