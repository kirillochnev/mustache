#include "job.hpp"

using namespace mustache;

void DefaultWorldFilterResult::apply(World& world, const ComponentIdMask& write_mask, WorldVersion prev_version) {
    (void ) write_mask;
    (void ) prev_version;
    filtered_archetypes.clear();
    total_entity_count = 0;

    auto& entities = world.entities();
    const auto num_archetypes = entities.getArchetypesCount();
    filtered_archetypes.reserve(num_archetypes);

    const auto cur_version = world.version();

    const auto filter_archetype = [&write_mask, cur_version, prev_version](Archetype& archetype) {
        const auto size = archetype.size();
        ArchetypeFilterResult item;
        item.archetype = &archetype;
        item.entities_count = size;
        item.blocks.push_back(EntityBlock{0, item.entities_count});
        ComponentIndexMask mask = archetype.makeComponentMask(write_mask);
        const auto last_index = archetype.lastChunkIndex();
        for (auto chunk_index = ChunkIndex::make(0); chunk_index <= last_index; ++chunk_index) {
            // TODO: update perf
            archetype.updateComponentVersions(mask, prev_version, mask, cur_version, chunk_index);
        }
        return item;
    };

    for(ArchetypeIndex index = ArchetypeIndex::make(0); index < ArchetypeIndex::make(num_archetypes); ++index) {
        auto& arch = entities.getArchetype(index);
        if(arch.size() > 0u && arch.isMatch(mask_)) {
            auto item = filter_archetype(arch);
            if (item.entities_count > 0) {
                total_entity_count += item.entities_count;
                filtered_archetypes.push_back(std::move(item));
            }
            /*total_entity_count += arch.size();
            ArchetypeFilterResult item;
            item.archetype = &arch;
            item.entities_count = arch.size();
            item.blocks.push_back(EntityBlock{0, item.entities_count});
            filtered_archetypes.push_back(item);*/
        }
    }
    /*
    if(filtered_.total_entity_count > 0) {
        last_update_version_ = world_version;
    }*/
}
