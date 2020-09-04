#pragma once

#include <type_traits>
#include <cstdint>

namespace mustache {
    enum class FunctionSafety : uint32_t {
         kUnsafe = 0,
         kSafe = 1,
         kDefault = kSafe
    };

    constexpr bool isSafe(FunctionSafety safety) noexcept {
        return safety == FunctionSafety::kSafe;
    }

}

#define MUSTACHE_INLINE inline __attribute__((always_inline))
#define MUSTACHE_RESTRICT_PTR __restrict