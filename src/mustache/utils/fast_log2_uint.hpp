#pragma once
#include <mustache/utils/default_settings.hpp>
#include <stdio.h>
#include <stdint.h>

namespace mustache {
    MUSTACHE_INLINE constexpr uint32_t fastLog2(uint32_t value) noexcept {
        const uint32_t magic_number = 0x07C4ACDD;
        const uint32_t tab32[32] = {
                0,  9,  1, 10, 13, 21,  2, 29,
                11, 14, 16, 18, 22, 25,  3, 30,
                8, 12, 20, 28, 15, 17, 24,  7,
                19, 27, 23,  6, 26,  5,  4, 31
        };

        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;

        return tab32[(value * magic_number) >> 27];
    }

    MUSTACHE_INLINE constexpr uint32_t fastLog2_2(uint32_t value) noexcept {
        return value == 0u ? 0u : static_cast<uint32_t>(31 - __builtin_clz(value));
    }
}
