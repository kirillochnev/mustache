#pragma once

#include <mustache/utils/default_settings.hpp>
#include <mustache/utils/memory_manager.hpp>
#include <mustache/utils/container_vector.hpp>
#include <cstddef>
#include <utility>

namespace mustache {
    template<typename _ElementType, typename _IndexType, bool _UseMemoryCustomAllocator>
    class ArrayWrapper {
        using _ArrayType = typename std::conditional<_UseMemoryCustomAllocator,
                mustache::vector<_ElementType, Allocator<_ElementType> >,
                mustache::vector<_ElementType> >::type;

        _ArrayType array_;
    public:
        template<typename... ARGS>
        ArrayWrapper(ARGS&&... args):
                array_(std::forward<ARGS>(args)...) {

        }

        decltype(auto) data() MUSTACHE_COPY_NOEXCEPT(_ArrayType::data) {
            return array_.data();
        }

        decltype(auto) data() const MUSTACHE_COPY_NOEXCEPT(_ArrayType::data) {
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
        auto resize(size_t new_size, const _ElementType& value) {
            return array_.resize(new_size, value);
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
        decltype(auto) back() MUSTACHE_COPY_NOEXCEPT(array_.back()) {
            return array_.back();
        }
        decltype(auto) front() MUSTACHE_COPY_NOEXCEPT(array_.front()) {
            return array_.front();
        }
        decltype(auto) back() const MUSTACHE_COPY_NOEXCEPT(array_.back()) {
            return array_.back();
        }
        decltype(auto) front() const MUSTACHE_COPY_NOEXCEPT(array_.front()) {
            return array_.front();
        }
        decltype(auto) clear() MUSTACHE_COPY_NOEXCEPT(array_.clear()){
            array_.clear();
        }

        bool empty() const noexcept {
            return array_.empty();
        }
        void shrink_to_fit() noexcept {
            array_.shrink_to_fit();
        }
    };
}
