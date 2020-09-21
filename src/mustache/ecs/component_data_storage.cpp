#include "component_data_storage.hpp"
#include <mustache/ecs/entity.hpp>

using namespace mustache;

void ComponentDataStorage::allocateChunk() {
    // TODO: use memory allocator
#ifdef _MSC_BUILD
    Chunk* chunk = new Chunk{};
#else
    Chunk* chunk = reinterpret_cast<Chunk*>(aligned_alloc(alignof(Entity), sizeof(Chunk)));
#endif
//    auto chunk = new Chunk{};
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
