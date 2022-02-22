#pragma once

#include <cstddef>
#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/default_settings.hpp>

namespace mustache {
template<typename T>
class Allocator;

#define MEMORY_MANAGER_COLLECT_STATISTICS 0
#if MEMORY_MANAGER_COLLECT_STATISTICS
#define MEMORY_MANAGER_STATISTICS_ARG_DECL , const char* file = MUSTACHE_FILE, uint32_t line = MUSTACHE_LINE
#define MEMORY_MANAGER_STATISTICS_FORWARD_ARG , file, line
#else
#define MEMORY_MANAGER_STATISTICS_ARG_DECL
#define MEMORY_MANAGER_STATISTICS_FORWARD_ARG

#endif
class MUSTACHE_EXPORT MemoryManager : mustache::Uncopiable {
    public:
        void* allocate(size_t size, size_t align = 0 MEMORY_MANAGER_STATISTICS_ARG_DECL) noexcept;
        void* allocateAndClear(size_t size, size_t align = 0) noexcept;
        void deallocate(void* ptr MEMORY_MANAGER_STATISTICS_ARG_DECL) noexcept;
        void showStatistic() const noexcept;
    template<typename T>
    Allocator<T> allocator() {
        return Allocator<T>{*this};
    }

    template<typename T>
    operator Allocator<T>() noexcept {
        return Allocator<T>(*this);
    }

    private:
    };

    template<typename T>
    class Allocator {
    public:
        Allocator() = default;

        Allocator(const Allocator& oth):
            manager_{oth.manager_} {

        }
        Allocator(Allocator&& oth):
            manager_{oth.manager_} {

        }

        Allocator& operator=(const Allocator& rhs) {
            return manager_ = rhs.manager_;
        }

        Allocator& operator=(Allocator&& rhs) {
            return manager_ = rhs.manager_;
        }

        bool operator==(const Allocator& rhs) const {
            return manager_ == rhs.manager_;
        }

        bool operator!=(const Allocator& rhs) const {
            return manager_ != rhs.manager_;
        }

        constexpr Allocator(MemoryManager& manager):
            manager_{&manager} {

        }

        T* allocate(size_t count MEMORY_MANAGER_STATISTICS_ARG_DECL) noexcept {
            auto ptr = manager_->allocate(sizeof(T) * count, alignof(T) MEMORY_MANAGER_STATISTICS_FORWARD_ARG);
            return static_cast<T*>(ptr);
        }

        void deallocate(T* ptr, size_t /*count*/ MEMORY_MANAGER_STATISTICS_ARG_DECL) noexcept {
            manager_->deallocate(ptr MEMORY_MANAGER_STATISTICS_FORWARD_ARG);
        }

        operator MemoryManager&() const noexcept {
            return *manager_;
        }
        using value_type = T;
    private:
        MemoryManager* manager_ = nullptr;
    };

#undef MEMORY_MANAGER_STATISTICS_ARG_DECL
#undef MEMORY_MANAGER_STATISTICS_FORWARD_ARG
}

