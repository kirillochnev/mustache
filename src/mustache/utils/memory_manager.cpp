#include "memory_manager.hpp"
#include <cstdlib>
#include <cstring>

void* mustache::MemoryManager::allocate(size_t size, size_t align) noexcept {
    if(align == 0) {
        return malloc(size);
    }
    return std::aligned_alloc(align, size);
}

void* mustache::MemoryManager::allocateAndClear(size_t size, size_t align) noexcept {
    void* result = allocate(size, align);
    memset(result, 0, size);
    return result;
}

void mustache::MemoryManager::deallocate(void* ptr) noexcept {
    free(ptr);
}
