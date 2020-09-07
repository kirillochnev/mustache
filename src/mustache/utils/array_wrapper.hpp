#pragma once

#include <mustache/utils/default_settings.hpp>
#include <utility>

namespace mustache {
    template<typename _ArrayType, typename _IndexType>
    class ArrayWrapper {
        _ArrayType array_;
    public:
        template<typename... ARGS>
        ArrayWrapper(ARGS&&... args):
                array_(std::forward<ARGS>(args)...) {

        }
        MUSTACHE_INLINE auto size() const noexcept {
            return array_.size();
        }
        MUSTACHE_INLINE decltype(auto) operator[](_IndexType i) noexcept {
            return array_[i.toInt()];
        }
        MUSTACHE_INLINE decltype(auto) operator[](_IndexType i) const noexcept {
            return array_[i.toInt()];
        }
        MUSTACHE_INLINE decltype(auto) at(_IndexType i) const noexcept {
            return array_.at(i.toInt());
        }
        MUSTACHE_INLINE decltype(auto) at(_IndexType i) noexcept {
            return array_.at(i.toInt());
        }
        MUSTACHE_INLINE decltype(auto) begin() noexcept {
            return array_.begin();
        }
        MUSTACHE_INLINE decltype(auto) begin() const noexcept {
            return array_.begin();
        }
        MUSTACHE_INLINE decltype(auto) rbegin() noexcept {
            return array_.rbegin();
        }
        MUSTACHE_INLINE decltype(auto) rbegin() const noexcept {
            return array_.rbegin();
        }
        MUSTACHE_INLINE decltype(auto) end() noexcept {
            return array_.end();
        }
        MUSTACHE_INLINE decltype(auto) end() const noexcept {
            return array_.end();
        }
        MUSTACHE_INLINE decltype(auto) rend() noexcept {
            return array_.end();
        }
        MUSTACHE_INLINE decltype(auto) rend() const noexcept {
            return array_.end();
        }
        MUSTACHE_INLINE decltype(auto) cbegin() const noexcept {
            return array_.cbegin();
        }
        MUSTACHE_INLINE decltype(auto) cend() const noexcept {
            return array_.cend();
        }
        template<typename T>
        MUSTACHE_INLINE decltype(auto) push_back(T&& item) {
            return array_.push_back(std::forward<T>(item));
        }
        template<typename... ARGS>
        MUSTACHE_INLINE decltype(auto) emplace_back(ARGS&&... args) {
            return array_.emplace_back(std::forward<ARGS>(args)...);
        }
        MUSTACHE_INLINE decltype(auto) erase(_IndexType i) {
            return array_.erase(array_.begin() + i.toInt());
        }
        MUSTACHE_INLINE bool has(_IndexType i) const noexcept {
            return array_.size() > i.toInt();
        }
        MUSTACHE_INLINE _IndexType back_index() const noexcept {
            _IndexType result;
            if (!array_.empty()) {
                result = _IndexType::make(array_.size() - 1);
            }
            return result;
        }
        MUSTACHE_INLINE decltype(auto) back() noexcept (noexcept(array_.back())) {
            return array_.back();
        }
        MUSTACHE_INLINE decltype(auto) front() noexcept (noexcept(array_.front())) {
            return array_.front();
        }
        MUSTACHE_INLINE decltype(auto) back() const noexcept (noexcept(array_.back())) {
            return array_.back();
        }
        MUSTACHE_INLINE decltype(auto) front() const noexcept (noexcept(array_.front())) {
            return array_.front();
        }
        MUSTACHE_INLINE decltype(auto) clear() noexcept (noexcept(array_.clear())){
            array_.clear();
        }
    };
}
