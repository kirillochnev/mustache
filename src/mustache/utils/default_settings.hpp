#pragma once

namespace mustache {
    constexpr bool kDefaultUnsafeValue = false;
}

#define MUSTACHE_INLINE inline __attribute__((always_inline))
#define MUSTACHE_RESTRICT_PTR __restrict