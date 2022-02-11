#pragma once

#include <stdexcept>

namespace mustache {
    struct MUSTACHE_EXPORT SharedComponentTag {};

    template <typename T>
    struct MUSTACHE_EXPORT TSharedComponentTag : public SharedComponentTag {};
}
