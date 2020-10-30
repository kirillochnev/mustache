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
#ifdef __clang__
    #define MUSTACHE_FILE __builtin_FILE()
    #define MUSTACHE_FUNCTION __builtin_FUNCTION()
    #define MUSTACHE_LINE __builtin_LINE()

    #define MUSTACHE_INLINE inline
    #define MUSTACHE_RESTRICT_PTR

    #define MUSTACHE_COPY_NOEXCEPT(exp)
#elif __GNUC__
    #include <experimental/source_location>

    #define MUSTACHE_FILE std::experimental::source_location::current().file_name()
    #define MUSTACHE_FUNCTION std::experimental::source_location::current().function_name()
    #define MUSTACHE_LINE std::experimental::source_location::current().line()

    #define MUSTACHE_INLINE inline __attribute__((always_inline))
    #define MUSTACHE_RESTRICT_PTR __restrict

    #define MUSTACHE_COPY_NOEXCEPT(exp) noexcept(noexcept(exp))
#elif _MSC_VER
    #define MUSTACHE_FILE __builtin_FILE()
    #define MUSTACHE_FUNCTION __builtin_FUNCTION()
    #define MUSTACHE_LINE __builtin_LINE()

    #define MUSTACHE_INLINE inline
    #define MUSTACHE_RESTRICT_PTR

    #define MUSTACHE_COPY_NOEXCEPT(exp)
#endif // _MSC_BUILD
