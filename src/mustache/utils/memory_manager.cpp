#include "memory_manager.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <cstring>
#include <malloc.h>

namespace {
    size_t total_size = 0;
}
void* mustache::MemoryManager::allocate(size_t size, size_t align) noexcept {
    total_size += size;
    const float mbytes = total_size / 1024.0f / 1024.0f;
    Logger{}.info("Total size: %fmb", mbytes);
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
    if (ptr) {
        total_size -= malloc_usable_size(ptr);
        free(ptr);
    }
}
