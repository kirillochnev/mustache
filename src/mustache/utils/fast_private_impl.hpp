#pragma once

#include <mustache/utils/unused.hpp>

#include <new>
#include <cstddef>
#include <utility>
#include <type_traits>

namespace mustache {
    template<typename T, size_t Size, size_t Align>
    class FastPimpl {
    public:
        template<typename... ARGS>
        explicit FastPimpl(ARGS&&... args) {
            static_assert(alignof(T) != 0 && ((Align % alignof(T)) == 0));
            static_assert(sizeof(T) <= Size);
            unused(new(&storage_) T(std::forward<ARGS>(args)...));
        }

        ~FastPimpl() {
            get()->~T();
        }

        T* operator->() noexcept {
            return get();
        }
        const T* operator->() const noexcept {
            return get();
        }

        T* get() {
            return std::launder(reinterpret_cast<T*>(&storage_));
        }
        const T* get() const {
            return std::launder(reinterpret_cast<const T*>(&storage_));
        }
    private:
        std::aligned_storage_t<Size, Align> storage_;
    };
}
