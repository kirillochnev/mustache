#pragma once

#include <mustache/utils/dll_export.h>

namespace mustache {
    /**
     * @class Uncopiable
     * @brief Represents a class that cannot be copied.
     *
     * The Uncopiable class provides a base class that cannot be copied using the copy constructor or assignment operator.
     */
    class MUSTACHE_EXPORT Uncopiable {
    public:
        Uncopiable() = default;
        Uncopiable(Uncopiable&&) = default;
        Uncopiable(const Uncopiable&) = delete;
        Uncopiable& operator=(const Uncopiable&) = delete;
        Uncopiable& operator=(Uncopiable&&) = default;
    };

}

