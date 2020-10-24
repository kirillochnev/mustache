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
#ifdef _MSC_BUILD
// NOTE: msvs does not support source_location, so some workaround is needed
#define MUSTACHE_FILE "unknown"
#define MUSTACHE_FUNCTION "unknown"
#define MUSTACHE_LINE 0

#define MUSTACHE_INLINE inline
#define MUSTACHE_RESTRICT_PTR
#else
#include <experimental/source_location>

#define MUSTACHE_FILE std::experimental::source_location::current().file_name()
#define MUSTACHE_FUNCTION std::experimental::source_location::current().function_name()
#define MUSTACHE_LINE std::experimental::source_location::current().line()

#define MUSTACHE_INLINE inline __attribute__((always_inline))
#define MUSTACHE_RESTRICT_PTR __restrict
#endif // _MSC_BUILD
