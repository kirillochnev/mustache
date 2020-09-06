#include "memory_manager.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>

#ifdef _MSC_BUILD
namespace {
    void* aligned_alloc(std::size_t size, std::size_t alignment) {
        if (alignment < alignof(void*)) {
            alignment = alignof(void*);
        }
        std::size_t space = size + alignment - 1;
        void* allocated_mem = ::operator new(space + sizeof(void*));
        void* aligned_mem = static_cast<void*>(static_cast<char*>(allocated_mem) + sizeof(void*));
        ////////////// #1 ///////////////
        std::align(alignment, size, aligned_mem, space);
        ////////////// #2 ///////////////
        *(static_cast<void**>(aligned_mem) - 1) = allocated_mem;
        ////////////// #3 ///////////////
        return aligned_mem;
    }
}
#endif
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
