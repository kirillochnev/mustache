#include "memory_manager.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <map>
#include <iostream>

#if MEMORY_MANAGER_COLLECT_STATISTICS
#define MEMORY_MANAGER_STATISTICS_ARG_DECL , const char* file, uint32_t line
namespace {
    size_t total_size = 0;
    std::map<void*, std::string> ptr_to_file;
    std::map<std::string, size_t> file_to_size;
}
#else
#define MEMORY_MANAGER_STATISTICS_ARG_DECL
#endif

void* mustache::MemoryManager::allocate(size_t size, size_t align MEMORY_MANAGER_STATISTICS_ARG_DECL) noexcept {
#ifdef _MSC_BUILD
    void* ptr = (align == 0) ? malloc(size) : _aligned_malloc(size, align);
#else
    void* ptr = (align == 0) ? malloc(size) : aligned_alloc(align, size);
#endif
#if MEMORY_MANAGER_COLLECT_STATISTICS
    total_size += size;
    const auto location = file + std::string(":") + std::to_string(line);
    file_to_size[location] += size;
    ptr_to_file[ptr] = location;
#endif
    return ptr;
}

void* mustache::MemoryManager::allocateAndClear(size_t size, size_t align) noexcept {
    void* result = allocate(size, align);
    memset(result, 0, size);
    return result;
}

void mustache::MemoryManager::deallocate(void* ptr MEMORY_MANAGER_STATISTICS_ARG_DECL) noexcept {
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
#if MEMORY_MANAGER_COLLECT_STATISTICS
    for (const auto& pair : file_to_size) {
        if (pair.second > 0) {
            const auto kbytes = pair.second / 1024.0f;
            std::cout << "File: " << pair.first << ", size: " << kbytes << "KB" << std::endl;
        }
    }
#else
    std::cerr << "Set MEMORY_MANAGER_COLLECT_STATISTICS 1" << std::endl;
#endif
}
