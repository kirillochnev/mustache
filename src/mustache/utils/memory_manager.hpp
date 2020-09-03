#pragma once

#include <cstddef>

namespace mustache {

    class MemoryManager {
    public:
        void* allocate(size_t size, size_t align = 0);
        void* allocateAndClear(size_t size, size_t align = 0);
        void deallocate(void* ptr);

    private:
    };
}

