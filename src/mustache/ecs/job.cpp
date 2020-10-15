#include "job.hpp"

using namespace mustache;

void DefaultWorldFilterResult::apply(World &world) {
    filtered_archetypes.clear();
    total_entity_count = 0;

    auto& entities = world.entities();
    const auto num_archetypes = entities.getArchetypesCount();
    filtered_archetypes.reserve(num_archetypes);

    for(ArchetypeIndex index = ArchetypeIndex::make(0); index < ArchetypeIndex::make(num_archetypes); ++index) {
        auto& arch = entities.getArchetype(index);
        if(arch.size() > 0u && arch.isMatch(mask_)) {
            total_entity_count += arch.size();
            ArchetypeFilterResult item;
            item.archetype = &arch;
            item.entities_count = arch.size();
            item.blocks.push_back(EntityBlock{0, item.entities_count});
            filtered_archetypes.push_back(item);
        }
    }
    /*
    if(filtered_.total_entity_count > 0) {
        last_update_version_ = world_version;
    }*/
}
