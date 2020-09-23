#pragma once

#include <mustache/utils/default_settings.hpp>
#include <mustache/ecs/id_deff.hpp>
#include <cstdint>
#include <cstddef>
#include <array>

namespace mustache {
    class ComponentDataStorage;

    class Chunk {
        friend ComponentDataStorage;
    public:
        enum : uint32_t {
            kChunkSize = 1024 * 1024
        };
//    private:
        Chunk() = default;
        Chunk(const Chunk&) = delete;
        template <typename T = std::byte>
        [[nodiscard]] T* data() noexcept {
            return reinterpret_cast<T*>(data_.data());
        }
        template <typename T = std::byte>
        [[nodiscard]] const T* data() const noexcept {
            return reinterpret_cast<const T*>(data_.data());
        }
        template <typename T = void>
        [[nodiscard]] T* dataPointerWithOffset(ComponentOffset offset) noexcept {
            return reinterpret_cast<T*>(offset.apply(data_.data()));
        }
        template <typename T = void>
        [[nodiscard]] const T* dataPointerWithOffset(ComponentOffset offset) const noexcept {
            return reinterpret_cast<const T*>(offset.apply(data_.data()));
        }

        [[nodiscard]] WorldVersion componentVersion(ComponentIndex index) const noexcept {
            return dataPointerWithOffset<WorldVersion>(ComponentOffset::make(0u))[index.toInt()];
        }

    private:
        std::array<std::byte, kChunkSize> data_;
    };
}
