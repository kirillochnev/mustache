#pragma once

#include <cstddef>
#include <mustache/utils/uncopiable.hpp>

namespace mustache {
template<typename T>
class Allocator;

class MemoryManager : mustache::Uncopiable {
    public:
        void* allocate(size_t size, size_t align = 0) noexcept;
        void* allocateAndClear(size_t size, size_t align = 0) noexcept;
        void deallocate(void* ptr) noexcept;

    template<typename T>
    Allocator<T> allocator() {
        return Allocator<T>{this};
    }

    template<typename T>
    operator Allocator<T>() noexcept {
        return Allocator<T>(this);
    }

    private:
    };

    template<typename T>
    class Allocator {
    public:
        constexpr Allocator(MemoryManager& manager):
            manager_{&manager} {

        }

        T* allocate(size_t count) noexcept {
            return static_cast<T*>(manager_->allocate(sizeof(T) * count, alignof(T)));
        }

        void deallocate(T* ptr, size_t /*count*/) noexcept {
            manager_->deallocate(ptr);
        }
        using value_type = T;
    private:
        MemoryManager* manager_;
    };
}

