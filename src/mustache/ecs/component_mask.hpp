#pragma once

#include <mustache/ecs/id_deff.hpp>

#include <bitset>
#include <vector>
#include <cstdint>
#include <initializer_list>

namespace mustache {


    struct ComponentMask {

        [[nodiscard]] uint64_t value() const noexcept {
            return uint_value_;
        }
        ComponentMask() = default;

        explicit ComponentMask(const std::initializer_list<ComponentId>& components) {
            for(auto id : components) {
                value_.set(id.toInt());
            }
            uint_value_ = value_.to_ullong();
        }

        [[nodiscard]] bool isEmpty() const noexcept {
            return !value_.any();
        }

        [[nodiscard]] std::vector<ComponentId > components() const noexcept {
            std::vector<ComponentId > result;
            for(size_t i = 0u; i < value_.size(); ++i) {
                if(value_.test(i)) {
                    result.push_back(ComponentId::make(i));
                }
            }
            return result;
        }

        [[nodiscard]] bool isMatch(const ComponentMask& rhs) const noexcept {
            return (uint_value_ & rhs.uint_value_) == rhs.uint_value_;
        }

        [[nodiscard]] bool has(ComponentId id) const noexcept{
            return value_.test(id.toInt());
        }

        void add(ComponentId id) noexcept {
            value_.set(id.toInt());
            uint_value_ = value_.to_ullong();
        }

        void set(ComponentId id, bool value) noexcept {
            value_.set(id.toInt(), value);
            uint_value_ = value_.to_ullong();
        }

        void reset(ComponentId id) noexcept {
            value_.reset(id.toInt());
            uint_value_ = value_.to_ullong();
        }

        [[nodiscard]] bool operator==(const ComponentMask& rhs) const noexcept {
            return value_ == rhs.value_;
        }

        [[nodiscard]] bool operator<(const ComponentMask& rhs) const noexcept {
            return uint_value_ < rhs.uint_value_;
        }


    private:
        std::bitset<64> value_;
        uint64_t uint_value_{0u};
    };

}