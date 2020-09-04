#pragma once

#include <type_traits>
#include <cstdint>

namespace mustache {

    struct Unsafe {
        static constexpr bool is_unsafe = true;
        static constexpr bool is_safe = false;
    };

    struct Safe {
        static constexpr bool is_unsafe = false;
        static constexpr bool is_safe = true;
    };

    template<typename T>
    constexpr bool IsSafe() noexcept {
        static_assert(std::is_same<T, Unsafe>::value || std::is_same<T, Safe>::value,
                "Safety type must be Safe or Unsafe");
        return T::is_safe;
    }

    using DefaultSafety = Safe;
}

#define MUSTACHE_INLINE inline __attribute__((always_inline))
#define MUSTACHE_RESTRICT_PTR __restrict