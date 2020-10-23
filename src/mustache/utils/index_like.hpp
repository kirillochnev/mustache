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
        [[nodiscard]] static constexpr _Type make(T1&& t) noexcept {
            _Type result;
            result.value_ = static_cast<T>(t);
            return result;
        }

        explicit constexpr IndexLike(IndexLike&& v) noexcept :
                value_{v.value_} {
        }

        explicit constexpr IndexLike(const IndexLike& v) noexcept :
                value_{v.value_} {
        }

        explicit constexpr IndexLike(T v) noexcept :
                value_{v} {
        }

        IndexLike& operator=(const IndexLike& rhs) noexcept {
            value_ = rhs.value_;
            return *this;
        }

        IndexLike& operator=(IndexLike&& rhs) noexcept {
            value_ = rhs.value_;
            return *this;
        }

        template <typename T1 = T>
        [[nodiscard]] constexpr T1 toInt() const noexcept {
            return static_cast<T1>(value_);
        }

        [[nodiscard]] static constexpr _Type null() noexcept {
            return make(kNull);
        }

        constexpr IndexLike() = default;
        [[nodiscard]] constexpr bool isValid() const {
            return value_ != kNull;
        }

        [[nodiscard]] constexpr bool isNull() const {
            return value_ == kNull;
        }

        [[nodiscard]] constexpr bool operator <(const IndexLike& rhs) const noexcept {
            return value_ < rhs.value_;
        }

        [[nodiscard]] constexpr bool operator >(const IndexLike& rhs) const noexcept {
            return value_ > rhs.value_;
        }

        [[nodiscard]] constexpr bool operator <=(const IndexLike& rhs) const noexcept {
            return value_ <= rhs.value_;
        }

        [[nodiscard]] constexpr bool operator >=(const IndexLike& rhs) const noexcept {
            return value_ >= rhs.value_;
        }

        [[nodiscard]] constexpr bool operator ==(const IndexLike& rhs) const noexcept {
            return value_ == rhs.value_;
        }

        [[nodiscard]] constexpr bool operator !=(const IndexLike& rhs) const noexcept {
            return value_ != rhs.value_;
        }
        [[nodiscard]] constexpr _Type next() const noexcept {
            return _Type::make(value_ + 1);
        }

        constexpr _Type& operator++() noexcept {
            ++value_;
            return *static_cast<_Type*>(this);
        }

        constexpr _Type operator++(int) noexcept {
            _Type copy = *this;
            ++value_;
            return copy;
        }
    protected:
        T value_{kNull};
    };
}
