#pragma once

#include <mustache/utils/default_settings.hpp>

namespace mustache {

    template <typename T, typename _Type, T NULL_VALUE = static_cast<T>(-1)>
    struct IndexLike {
        using ValueType = T;
        enum : T {
            kNull = NULL_VALUE
        };

        template <typename T1>
        [[nodiscard]] MUSTACHE_INLINE static constexpr _Type make(T1&& t) noexcept {
            _Type result;
            result.value_ = static_cast<T>(t);
            return result;
        }

        explicit MUSTACHE_INLINE constexpr IndexLike(T v) noexcept :
                value_{v} {
        }

        template <typename T1 = T>
        [[nodiscard]] MUSTACHE_INLINE constexpr T1 toInt() const noexcept {
            return static_cast<T1>(value_);
        }

        [[nodiscard]] MUSTACHE_INLINE static constexpr _Type null() noexcept {
            return make(kNull);
        }

        constexpr IndexLike() = default;
        [[nodiscard]] MUSTACHE_INLINE constexpr bool isValid() const {
            return value_ != kNull;
        }

        [[nodiscard]] MUSTACHE_INLINE constexpr bool isNull() const {
            return value_ == kNull;
        }

        [[nodiscard]] MUSTACHE_INLINE constexpr bool operator <(const IndexLike& rhs) const noexcept {
            return value_ < rhs.value_;
        }

        [[nodiscard]] MUSTACHE_INLINE constexpr bool operator >(const IndexLike& rhs) const noexcept {
            return value_ > rhs.value_;
        }

        [[nodiscard]] MUSTACHE_INLINE constexpr bool operator <=(const IndexLike& rhs) const noexcept {
            return value_ <= rhs.value_;
        }

        [[nodiscard]] MUSTACHE_INLINE constexpr bool operator >=(const IndexLike& rhs) const noexcept {
            return value_ <= rhs.value_;
        }

        [[nodiscard]] MUSTACHE_INLINE constexpr bool operator ==(const IndexLike& rhs) const noexcept {
            return value_ == rhs.value_;
        }

        [[nodiscard]] MUSTACHE_INLINE constexpr bool operator !=(const IndexLike& rhs) const noexcept {
            return value_ != rhs.value_;
        }
        [[nodiscard]] MUSTACHE_INLINE constexpr _Type next() const noexcept {
            return _Type::make(value_ + 1);
        }

        MUSTACHE_INLINE constexpr _Type& operator++() noexcept {
            ++value_;
            return *static_cast<_Type*>(this);
        }

        MUSTACHE_INLINE constexpr _Type operator++(int) noexcept {
            _Type copy = *this;
            ++value_;
            return copy;
        }
    protected:
        T value_{kNull};
    };

}