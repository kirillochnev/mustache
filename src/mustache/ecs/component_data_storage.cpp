#include "component_data_storage.hpp"
#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <memory>
#include <mustache/utils/logger.hpp>

using namespace mustache;
namespace {
    constexpr uint32_t getChunkAlign(uint32_t num_of_components) noexcept {
        const auto is_world_id_align_ok = (num_of_components * sizeof(WorldVersion)
                + alignof(WorldVersion)) % alignof(Entity) == 0;
        return is_world_id_align_ok ? alignof(WorldVersion) : alignof(Entity);
    }
}

ComponentDataStorage::ComponentDataStorage(const ComponentMask& mask) {
    component_getter_info_.reserve(mask.componentsCount());

    uint32_t element_size = sizeof(Entity);

    mask.forEachComponent([this, &element_size](ComponentId id) {
        const auto& info = ComponentFactory::componentInfo(id);
        ComponentDataGetter getter;
        getter.offset = ComponentOffset::makeAligned(ComponentOffset::make(element_size), info.align);
        getter.size = info.size;
        element_size = getter.offset.add(info.size).toInt();
        component_getter_info_.push_back(getter);
    });

    const auto entity_offset = entityOffset();

    chunk_capacity_ = ChunkCapacity::make((Chunk::kChunkSize - entity_offset.toInt()) / element_size);

    for (auto& getter : component_getter_info_) {
        getter.offset = entity_offset.add(getter.offset.toInt() * chunk_capacity_.toInt());
    }
}

void ComponentDataStorage::allocateChunk() {
    // TODO: use memory allocator
#ifdef _MSC_BUILD
    Chunk* chunk = new Chunk{};
#else
    const auto chunk_alignment = getChunkAlign(componentsCount());
    Chunk* chunk = reinterpret_cast<Chunk*>(aligned_alloc(chunk_alignment, sizeof(Chunk)));
#endif
    chunks_.push_back(chunk);
}

void ComponentDataStorage::freeChunk(Chunk* chunk) noexcept {
    // TODO: use memory allocator
#ifdef _MSC_BUILD
    delete chunk;
#else
    free(chunk);
#endif
}
