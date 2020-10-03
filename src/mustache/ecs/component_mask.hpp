#pragma once

#include <mustache/ecs/id_deff.hpp>

#include <bitset>
#include <vector>
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

        [[nodiscard]] uint64_t toUInt64() const noexcept {
            return static_cast<uint64_t>(value_.to_ullong());
        }

        [[nodiscard]] bool isEmpty() const noexcept {
            return !value_.any();
        }

        [[nodiscard]] std::vector<_ItemType > items() const noexcept {
            std::vector<_ItemType > result;
            forEachItem([&result](auto id) {
                result.push_back(id);
            });
            return result;
        }

        template<typename _F>
        void forEachItem(_F&& function) const noexcept {
            for(size_t i = 0u; i < value_.size(); ++i) {
                if(value_.test(i)) {
                    function(ComponentId::make(i));
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

        void set(_ItemType item, bool value) noexcept {
            value_.set(item.toInt(), value);
        }

        void reset(_ItemType item) noexcept {
            value_.reset(item.toInt());
        }

        [[nodiscard]] bool operator==(const ComponentMask& rhs) const noexcept {
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
        std::string toString() const noexcept;
        using ComponentMask::ComponentMask;
    };

    struct ComponentIndexMask : public ComponentMask<ComponentIndex, 64> {
        using ComponentMask::ComponentMask;
    };
}
