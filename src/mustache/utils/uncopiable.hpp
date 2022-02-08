#pragma once

#include <mustache/utils/dll_export.h>

namespace mustache {
    class MUSTACHE_EXPORT Uncopiable {
    public:
        Uncopiable() = default;
        Uncopiable(Uncopiable&&) = default;
        Uncopiable(const Uncopiable&) = delete;
        Uncopiable& operator=(const Uncopiable&) = delete;
        Uncopiable& operator=(Uncopiable&&) = default;
    };

}

