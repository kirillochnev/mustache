#include "memory_manager.hpp"
#include <mustache/utils/logger.hpp>
#include <mustache/utils/profiler.hpp>
#include <mustache/utils/container_map.hpp>

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>
#ifdef __APPLE__
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif


#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

namespace {
    std::mutex pages_mutex;
    std::unordered_map<void*, size_t> pages;
}

using namespace mustache;

void* MemoryManager::allocate(size_t size, size_t align) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3("MemoryManager::allocate");
#ifdef _MSC_BUILD
#define ALIGNED_ALLOC(size, align) _aligned_malloc(size, align)
#elif defined(ANDROID)
#define ALIGNED_ALLOC(size, align) memalign(align, size)
#else
#define ALIGNED_ALLOC(size, align) aligned_alloc(align, size)
#endif
    constexpr size_t min_align = 8;
    if (align < min_align) {
        align = min_align;
        size = (size + align - 1) & ~(align - 1);
    }
#ifdef __APPLE__
    void* ptr = (align < 8) ? malloc(size) : ALIGNED_ALLOC(size, align);
#else
    void* ptr = (align == 0) ? malloc(size) : ALIGNED_ALLOC(size, align);
#endif

#undef ALIGNED_ALLOC

    if (ptr == nullptr) {
        ptr = malloc(size);
    }
    return ptr;
}

void* MemoryManager::allocateAndClear(size_t size, size_t align) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3("MemoryManager::allocateAndClear");
    void* result = allocate(size, align);
    memset(result, 0, size);
    return result;
}

void MemoryManager::deallocate(void* ptr) noexcept {
    MUSTACHE_PROFILER_BLOCK_LVL_3("MemoryManager::deallocate");
    if (ptr) {
#ifdef _MSC_BUILD
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
}

void* MemoryManager::allocateSmart(size_t size, size_t align, bool allow_pages, bool allow_large_pages) noexcept {
    void* ptr = nullptr;
    if (allow_large_pages && size >= large_page_size) {
        std::lock_guard lock {pages_mutex};
#ifdef _WIN32
        ptr = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
#else
        ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        if (ptr == MAP_FAILED) ptr = nullptr;
#endif
        if (ptr) {
            pages[ptr] = size;
            return ptr;
        }
    }

    if (allow_pages) {
        std::lock_guard lock {pages_mutex};
#ifdef _WIN32
        ptr = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
        ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) ptr = nullptr;
#endif
        if (ptr) {
            pages[ptr] = size;
            return ptr;
        }
    }
    return allocate(size, align);
}

void MemoryManager::deallocateSmart(void* ptr) noexcept {
    if (reinterpret_cast<uintptr_t>(ptr) % page_size == 0) {
        const auto find_res = pages.find(ptr);
        if (find_res != pages.end()) {
            std::lock_guard lock {pages_mutex};
#ifdef _WIN32
            VirtualFree(ptr, 0, MEM_RELEASE);
#else
            munmap(ptr, find_res->second);
#endif
            pages.erase(find_res);
            return;
        }
    }

    deallocate(ptr);
}

