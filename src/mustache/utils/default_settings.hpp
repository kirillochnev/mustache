#pragma once

#include <mustache/utils/dll_export.h>

#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <new>

namespace mustache {
    enum class FunctionSafety : uint32_t {
        kUnsafe = 0,
        kSafe = 1,
        kDefault = kSafe
    };

    constexpr bool isSafe(FunctionSafety safety) noexcept {
        return safety == FunctionSafety::kSafe;
    }
    static constexpr size_t CacheLineSize = 64;//std::hardware_destructive_interference_size;
    template <typename T>
    struct fits_in_min_cache_lines {
        static constexpr std::size_t S = sizeof(T);
        static constexpr std::size_t A = alignof(T);
        static constexpr std::size_t M = (S + CacheLineSize - 1) / CacheLineSize;

        static constexpr std::size_t maxOffsetFor(std::size_t X) {
            return ((M * CacheLineSize - 1) / X) * X;
        }

        static constexpr bool value =
                (maxOffsetFor(A) + S) <= (M * CacheLineSize);

        // Заменили лямбду на static constexpr функцию
        static constexpr std::size_t compute_recommended_alignment() {
            std::size_t x = A;
            while ((maxOffsetFor(x) + S) > (M * CacheLineSize)) {
                x *= 2;
            }
            return x;
        }

        static constexpr std::size_t recommended_alignment =
                compute_recommended_alignment();

        static constexpr void check() noexcept {
            static_assert(recommended_alignment == alignof(T));
        }
    };
}
#ifdef __clang__
#if __clang_major__ > 10
    #define MUSTACHE_FILE __builtin_FILE()
    #define MUSTACHE_FUNCTION __builtin_FUNCTION()
    #define MUSTACHE_LINE __builtin_LINE()
#else
// TODO: fix me!
    #define MUSTACHE_FILE ""
    #define MUSTACHE_FUNCTION ""
    #define MUSTACHE_LINE 0u
#endif
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
#if _MSC_VER > 1900
    #define MUSTACHE_FILE __builtin_FILE()
    #define MUSTACHE_FUNCTION __builtin_FUNCTION()
    #define MUSTACHE_LINE __builtin_LINE()
#else
    #define MUSTACHE_FILE ""
    #define MUSTACHE_FUNCTION ""
    #define MUSTACHE_LINE 0u
#endif
    #define MUSTACHE_INLINE inline
    #define MUSTACHE_RESTRICT_PTR

    #define MUSTACHE_COPY_NOEXCEPT(exp)
#endif // _MSC_BUILD
