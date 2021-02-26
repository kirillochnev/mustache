#pragma once

#include <mustache/ecs/id_deff.hpp>
#include <mustache/utils/invoke.hpp>

#include <bitset>
#include <vector>
#include <memory>
#include <cstdint>
#include <initializer_list>

namespace mustache {

    template<typename _ItemType, size_t _MaxElements>
    struct ComponentMask {

        ComponentMask() = default;

        explicit ComponentMask(const std::initializer_list<_ItemType>& items) {
            for(auto id : items) {
                value_.set(id.toInt());
            }
        }

        [[nodiscard]] static constexpr size_t maxElementsCount() noexcept {
            return _MaxElements;
        }

        [[nodiscard]] uint64_t toUInt64() const noexcept {
            return static_cast<uint64_t>(value_.to_ullong());
        }

        [[nodiscard]] bool isEmpty() const noexcept {
            return !value_.any();
        }

        [[nodiscard]] std::vector<_ItemType > items() const noexcept {
            std::vector<_ItemType > result;
            for (size_t i = 0u; i < value_.size(); ++i) {
                if (value_.test(i)) {
                    result.push_back(_ItemType::make(i));
                }
            }
            return result;
        }

        template<typename _F>
        void forEachItem(_F&& function) const noexcept {
            for (size_t i = 0u; i < value_.size(); ++i) {
                using ResultType = decltype(function(_ItemType::make(0)));
                constexpr bool can_be_interrupted = std::is_same<ResultType, bool>::value;
                if constexpr (can_be_interrupted) {
                    if (value_.test(i)) {
                        if (!function(_ItemType::make(i))) {
                            return;
                        }
                    }
                } else {
                    if (value_.test(i)) {
                        function(_ItemType::make(i));
                    }
                }
            }
        }

        [[nodiscard]] bool isMatch(const ComponentMask& rhs) const noexcept {
            return (toUInt64() & rhs.toUInt64()) == rhs.toUInt64();
        }

        [[nodiscard]] bool has(_ItemType item) const noexcept{
            return value_.test(item.toInt());
        }

        void add(_ItemType item) noexcept {
            value_.set(item.toInt());
        }

        [[nodiscard]] ComponentMask merge(const ComponentMask& extra) const noexcept {
            ComponentMask result;
            result.value_ = value_ | extra.value_;
            return result;
        }

        void set(_ItemType item, bool value) noexcept {
            value_.set(item.toInt(), value);
        }

        void reset(_ItemType item) noexcept {
            value_.reset(item.toInt());
        }

        [[nodiscard]] bool operator==(const ComponentMask& rhs) const noexcept {
            return value_ == rhs.value_;
        }

        [[nodiscard]] bool operator!=(const ComponentMask& rhs) const noexcept {
            return value_ == rhs.value_;
        }

        [[nodiscard]] bool operator<(const ComponentMask& rhs) const noexcept {
            return toUInt64() < rhs.toUInt64();
        }

        [[nodiscard]] uint32_t componentsCount() const noexcept {
            return static_cast<uint32_t>(value_.count());
        }
    private:
        std::bitset<_MaxElements> value_;
    };

    struct ComponentIdMask : public ComponentMask<ComponentId, 64> {
        ComponentIdMask(const ComponentMask<ComponentId, 64>& oth):
                ComponentMask{oth} {

        }
        ComponentIdMask(ComponentMask<ComponentId, 64>&& oth):
                ComponentMask{std::move(oth)} {

        }

        std::string toString() const noexcept;
        using ComponentMask::ComponentMask;
    };

    struct SharedComponentIdMask : public ComponentMask<SharedComponentId, 64> {
        SharedComponentIdMask(const ComponentMask<SharedComponentId, 64>& oth):
                ComponentMask{oth} {

        }
        SharedComponentIdMask(ComponentMask<SharedComponentId, 64>&& oth):
                ComponentMask{std::move(oth)} {

        }

        std::string toString() const noexcept;
        using ComponentMask::ComponentMask;
    };

    struct SharedComponentTag;

    using SharedComponentPtr = std::shared_ptr<const SharedComponentTag>;
    
    using SharedComponentsData = std::vector<SharedComponentPtr>;

    struct SharedComponentsInfo {

        void add(SharedComponentId id, const SharedComponentPtr& value) {
            if (!mask_.has(id)) {
                mask_.set(id, true);
                ids_.push_back(id);
                data_.push_back(value);
            } else {
                data_[indexOf(id).toInt()] = value;
            }
        }

        void remove(SharedComponentId id) {
            if (has(id)) {
                const auto index = indexOf(id);
                data_.erase(data_.begin() + index.toInt());
                ids_.erase(ids_.begin() + index.toInt());
            }
            mask_.set(id, false);
        }

        template <typename _F>
        void forEach(_F&& function) const noexcept {
            uint32_t index = 0;
            for (auto id : ids_) {
                return invoke(function, id, data_[index++]);
            }
        }
        [[nodiscard]] bool has(SharedComponentId id) const noexcept {
            return mask_.has(id);
        }

        [[nodiscard]] bool isMatch(const SharedComponentIdMask& mask) const noexcept {
            return mask_.isMatch(mask);
        }
        [[nodiscard]] const SharedComponentIdMask& mask() const noexcept {
            return mask_;
        }

        [[nodiscard]] const SharedComponentsData& data() const noexcept {
            return data_;
        }

        [[nodiscard]] SharedComponentIndex indexOf(SharedComponentId id) const noexcept {
            for (uint32_t i = 0; i < ids_.size(); ++i) {
                if (ids_[i] == id) {
                    return SharedComponentIndex::make(i);
                }
            }
            return SharedComponentIndex::null();
        }

        std::shared_ptr<const SharedComponentTag> get(SharedComponentIndex index) const noexcept {
            return data_[index.toInt()];
        }

    private:
        SharedComponentIdMask mask_;
        std::vector<SharedComponentId> ids_;
        SharedComponentsData data_;

    };

    struct ComponentIndexMask : public ComponentMask<ComponentIndex, 64> {
        using ComponentMask::ComponentMask;
    };
}
