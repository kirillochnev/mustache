#pragma once

#include <stdexcept>

namespace mustache {
    struct SharedComponentTag {};

    template <typename T>
    struct TSharedComponentTag : public SharedComponentTag {};
}
