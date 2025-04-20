//
// Created by kirill on 18.04.25.
//

#pragma once

#include <cstddef>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>

namespace mustache {

    template <typename T>
    class Span {
    public:
        using element_type     = T;
        using value_type       = std::remove_cv_t<T>;
        using size_type        = std::size_t;
        using difference_type  = std::ptrdiff_t;
        using pointer          = T*;
        using const_pointer    = const T*;
        using reference        = T&;
        using const_reference  = const T&;
        using iterator         = T*;
        using const_iterator   = const T*;

        constexpr Span() noexcept
                : ptr_(nullptr), size_(0) {}

        constexpr Span(pointer ptr, size_type count) noexcept
                : ptr_(ptr), size_(count) {}

        template <size_type N>
        constexpr Span(element_type (&arr)[N]) noexcept  // массив C
                : ptr_(arr), size_(N) {}

        template <typename Container>
        constexpr Span(Container& c) noexcept               // любой контейнер с .data() и .size()
                : ptr_(c.data()), size_(c.size()) {}

        constexpr pointer data() const noexcept       { return ptr_; }
        constexpr size_type size() const noexcept     { return size_; }
        constexpr bool empty() const noexcept         { return size_ == 0; }

        constexpr reference operator[](size_type idx) const { return ptr_[idx]; }
        constexpr reference at(size_type idx) const {
            if (idx >= size_) throw std::out_of_range("Span::at");
            return ptr_[idx];
        }

        constexpr iterator begin() const noexcept { return ptr_; }
        constexpr iterator end()   const noexcept { return ptr_ + size_; }

        constexpr Span subspan(size_type offset, size_type count) const {
            if (offset > size_ || offset + count > size_)
                throw std::out_of_range("Span::subspan");
            return Span(ptr_ + offset, count);
        }

        constexpr Span first(size_type count) const {
            return subspan(0, count);
        }

        constexpr Span last(size_type count) const {
            return subspan(size_ - count, count);
        }
    private:
        pointer   ptr_;
        size_type size_;
    };
}
