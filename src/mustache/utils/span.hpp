//
// Created by kirill on 18.04.25.
//

#pragma once

#include <span>
#include <cstddef>

namespace mustache {
    template<typename T, size_t _Extent = std::dynamic_extent>
    using Span = std::span<T, _Extent>;
}
