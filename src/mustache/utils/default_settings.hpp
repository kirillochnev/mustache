#pragma once

#include <mustache/utils/dll_export.h>

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

#define MUSTACHE_STRINGIFY_HELPER(x) #x
#define MUSTACHE_STRINGIFY(x)        MUSTACHE_STRINGIFY_HELPER(x)

#if defined(_MSC_VER)
// MSVC
  #define MUSTACHE_IVDEP     __pragma(loop(ivdep))
  #define MUSTACHE_UNROLL(N) __pragma(loop(unroll(N)))

#elif defined(__clang__)
// Clang/LLVM
  #define MUSTACHE_IVDEP     _Pragma("clang loop vectorize(enable) interleave(enable)")
  #define MUSTACHE_UNROLL(N) _Pragma(MUSTACHE_STRINGIFY(clang loop unroll_count(N)))

#elif defined(__GNUC__)
// GCC
#define MUSTACHE_IVDEP     _Pragma("GCC ivdep")
#define MUSTACHE_UNROLL(N) _Pragma(MUSTACHE_STRINGIFY(GCC unroll N))

#elif defined(__INTEL_COMPILER)
// Intel C/C++ Compiler
  #define MUSTACHE_IVDEP     _Pragma("ivdep")
  #define MUSTACHE_UNROLL(N) _Pragma(MUSTACHE_STRINGIFY(unroll N))

#else
  // Unknown compiler: no-op
  #define MUSTACHE_IVDEP
  #define MUSTACHE_UNROLL(N)
#endif
