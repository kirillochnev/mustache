#pragma once

#include <mustache/utils/dll_export.h>

#include <string>

#ifdef _MSC_BUILD
#define MUSTACHE_FUNCTION_SIGNATURE __FUNCSIG__
#else
#define MUSTACHE_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif

namespace mustache {

    namespace detail {
        MUSTACHE_EXPORT std::string make_type_name_from_func_name(const char* func_name) noexcept;
    }

    template <typename T>
    const std::string& type_name() noexcept {
        static std::string result = detail::make_type_name_from_func_name(MUSTACHE_FUNCTION_SIGNATURE);
        return result;
    }

}
