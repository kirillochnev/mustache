#pragma once

#include <mustache/utils/default_settings.hpp>
#include <mustache/utils/memory_manager.hpp>
#include <cstddef>
#include <utility>
#include <vector>

namespace mustache {
    template<typename _ElementType, typename _IndexType, bool _UseMemoryCustomAllocator = false>
    class ArrayWrapper {
        using _ArrayType = typename std::conditional<_UseMemoryCustomAllocator,
                std::vector<_ElementType, Allocator<_ElementType> >,
                std::vector<_ElementType> >::type;

        _ArrayType array_;
    public:
        template<typename... ARGS>
        ArrayWrapper(ARGS&&... args):
                array_(std::forward<ARGS>(args)...) {

        }

        decltype(auto) data()
#ifndef _MSC_BUILD
            noexcept (noexcept(_ArrayType::data))
#endif
        {
            return array_.data();
        }

        decltype(auto) data() const
#ifndef _MSC_BUILD
            noexcept (noexcept(_ArrayType::data))
#endif
        {
            return array_.data();
        }
        auto size() const noexcept {
            return array_.size();
        }
        auto capacity() const noexcept {
            return array_.capacity();
        }
        auto resize(size_t new_size) {
            return array_.resize(new_size);
        }
        auto reserve(size_t new_capacity) {
            return array_.reserve(new_capacity);
        }
        decltype(auto) operator[](_IndexType i) noexcept {
            return array_[i.toInt()];
        }
        decltype(auto) operator[](_IndexType i) const noexcept {
            return array_[i.toInt()];
        }
        decltype(auto) at(_IndexType i) const noexcept {
            return array_.at(i.toInt());
        }
        decltype(auto) at(_IndexType i) noexcept {
            return array_.at(i.toInt());
        }
        decltype(auto) begin() noexcept {
            return array_.begin();
        }
        decltype(auto) begin() const noexcept {
            return array_.begin();
        }
        decltype(auto) rbegin() noexcept {
            return array_.rbegin();
        }
        decltype(auto) rbegin() const noexcept {
            return array_.rbegin();
        }
        decltype(auto) end() noexcept {
            return array_.end();
        }
        decltype(auto) end() const noexcept {
            return array_.end();
        }
        decltype(auto) rend() noexcept {
            return array_.end();
        }
        decltype(auto) rend() const noexcept {
            return array_.end();
        }
        decltype(auto) cbegin() const noexcept {
            return array_.cbegin();
        }
        decltype(auto) cend() const noexcept {
            return array_.cend();
        }
        decltype(auto) pop_back() {
            return array_.pop_back();
        }
        template<typename T>
        decltype(auto) push_back(T&& item) {
            return array_.push_back(std::forward<T>(item));
        }
        template<typename... ARGS>
        decltype(auto) emplace_back(ARGS&&... args) {
            return array_.emplace_back(std::forward<ARGS>(args)...);
        }
        decltype(auto) erase(_IndexType i) {
            return array_.erase(array_.begin() + i.toInt());
        }
        bool has(_IndexType i) const noexcept {
            return array_.size() > i.toInt();
        }
        _IndexType back_index() const noexcept {
            _IndexType result;
            if (!array_.empty()) {
                result = _IndexType::make(array_.size() - 1);
            }
            return result;
        }
        decltype(auto) back() noexcept (noexcept(array_.back())) {
            return array_.back();
        }
        decltype(auto) front() noexcept (noexcept(array_.front())) {
            return array_.front();
        }
        decltype(auto) back() const noexcept (noexcept(array_.back())) {
            return array_.back();
        }
        decltype(auto) front() const noexcept (noexcept(array_.front())) {
            return array_.front();
        }
        decltype(auto) clear() noexcept (noexcept(array_.clear())){
            array_.clear();
        }

        bool empty() const noexcept {
            return array_.empty();
        }
    };
}
