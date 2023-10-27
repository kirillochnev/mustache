#pragma once

namespace mustache {
    template <typename... ARGS>
    constexpr inline void unused(const ARGS&...) noexcept {
    }
}
