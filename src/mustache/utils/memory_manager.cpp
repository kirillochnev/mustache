#include "memory_manager.hpp"
#include <mustache/utils/logger.hpp>
#include <mustache/utils/profiler.hpp>
#include <mustache/utils/container_map.hpp>

#include <cstdlib>
#include <cstring>
#ifdef __APPLE__
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

#if MEMORY_MANAGER_COLLECT_STATISTICS
#define MEMORY_MANAGER_STATISTICS_ARG_DECL , const char* file, uint32_t line
namespace {
    size_t total_size = 0;
    mustache::map<void*, std::string> ptr_to_file;
    mustache::map<std::string, size_t> file_to_size;
}
#else
#define MEMORY_MANAGER_STATISTICS_ARG_DECL
#endif

void* mustache::MemoryManager::allocate(size_t size, size_t align MEMORY_MANAGER_STATISTICS_ARG_DECL) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3("MemoryManager::allocate");
#ifdef _MSC_BUILD
    #define ALIGNED_ALLOC(size, align) _aligned_malloc(size, align)
#elif defined(ANDROID)
    #define ALIGNED_ALLOC(size, align) memalign(align, size)
#else
    #define ALIGNED_ALLOC(size, align) aligned_alloc(align, size)
#endif

#ifdef __APPLE__
    void* ptr = (align < 8) ? malloc(size) : ALIGNED_ALLOC(size, align);
#else
    void* ptr = (align == 0) ? malloc(size) : ALIGNED_ALLOC(size, align);
#endif
    
#undef ALIGNED_ALLOC

#if MEMORY_MANAGER_COLLECT_STATISTICS
    total_size += size;
    const auto location = file + std::string(":") + std::to_string(line);
    file_to_size[location] += size;
    ptr_to_file[ptr] = location;
#endif
    return ptr;
}

void* mustache::MemoryManager::allocateAndClear(size_t size, size_t align) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3("MemoryManager::allocateAndClear");
    void* result = allocate(size, align);
    memset(result, 0, size);
    return result;
}

void mustache::MemoryManager::deallocate(void* ptr MEMORY_MANAGER_STATISTICS_ARG_DECL) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3("MemoryManager::deallocate");
    if (ptr) {
#if MEMORY_MANAGER_COLLECT_STATISTICS
        (void )file;
        (void ) line;
        const auto size = malloc_usable_size(ptr);
        total_size -= size;
        file_to_size[ptr_to_file[ptr]] -= size;
#endif

#ifdef _MSC_BUILD
        _aligned_free(ptr);
#else
        free(ptr);
#endif

    }
}

void mustache::MemoryManager::showStatistic() const noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3("MemoryManager::showStatistic");
#if MEMORY_MANAGER_COLLECT_STATISTICS
    for (const auto& pair : file_to_size) {
        if (pair.second > 0) {
            const auto mbytes = static_cast<float>(pair.second) / 1024.0f / 1024.f;
            Logger{}.info("File: %s, size: %dMB", pair.first.c_str(), mbytes);
        }
    }
#else
    Logger{}.error("Set MEMORY_MANAGER_COLLECT_STATISTICS 1");
#endif
}
