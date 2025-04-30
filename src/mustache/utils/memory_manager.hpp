#pragma once

#include <cstddef>
#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/default_settings.hpp>

namespace mustache {
    template<typename T>
    class Allocator;

    class MUSTACHE_EXPORT MemoryManager : mustache::Uncopiable {
    public:
        static constexpr size_t page_size = 1 << 12;
        static constexpr size_t large_page_size = 1 << 21;

        [[nodiscard]] static constexpr size_t pageSize(bool large = false) noexcept {
            return large ? large_page_size : page_size;
        }
        [[nodiscard]] void* allocate(size_t size, size_t align = 0) noexcept;
        [[nodiscard]] void* allocateAndClear(size_t size, size_t align = 0) noexcept;
        [[nodiscard]] void* allocateSmart(size_t size, size_t align = 0, bool allow_pages = true, bool allow_large_pages = true) noexcept;
        void deallocateSmart(void* ptr) noexcept;
        void deallocate(void* ptr) noexcept;
        template<typename T>
        Allocator<T> allocator() {
            return Allocator<T>{*this};
        }

        template<typename T>
        operator Allocator<T>() noexcept {
            return Allocator<T>(*this);
        }
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

        T* allocate(size_t count) noexcept {
            auto ptr = manager_->allocate(sizeof(T) * count, alignof(T));
            return static_cast<T*>(ptr);
        }

        void deallocate(T* ptr, size_t /*count*/) noexcept {
            manager_->deallocate(ptr);
        }

        operator MemoryManager&() const noexcept {
            return *manager_;
        }
        using value_type = T;
    private:
        MemoryManager* manager_ = nullptr;
    };
}
