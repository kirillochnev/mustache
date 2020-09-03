#include <cstdlib>
#include <cstring>
#include "memory_manager.hpp"

void* mustache::MemoryManager::allocate(size_t size, size_t align) {
    if(align == 0) {
        return malloc(size);
    }
    return aligned_alloc(align, size);
}

void* mustache::MemoryManager::allocateAndClear(size_t size, size_t align) {
    void* result = allocate(size, align);
    memset(result, 0, size);
    return result;
}

void mustache::MemoryManager::deallocate(void* ptr) {
    free(ptr);
}
