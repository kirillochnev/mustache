#pragma once

#include <mustache/ecs/entity.hpp>
#include <stdexcept>

namespace mustache {

    class MUSTACHE_EXPORT EntityGroup {
    public:
        EntityGroup(mustache::vector<Entity>&& fragmented, uint32_t first, uint32_t count):
                fragmented_(std::move(fragmented)),
                first_{first},
                count_{count} {

        };
        EntityGroup(const mustache::vector<Entity>& fragmented, uint32_t first, uint32_t count):
                fragmented_(fragmented),
                first_{first},
                count_{count} {

        };

        struct Iterator {
            const EntityGroup* owner;
            uint32_t i;
            bool operator!=(const Iterator& rhs) const noexcept {
                return i != rhs.i || owner != rhs.owner;
            }
            Iterator& operator++() {
                ++i;
                return *this;
            }
            Entity operator*() const {
                return owner->at(i);
            }
            Entity operator->() const {
                return owner->at(i);
            }
            int32_t operator-(const Iterator& rhs) const noexcept {
                return static_cast<int32_t>(i) - static_cast<int32_t>(rhs.i);
            }
        };

        [[nodiscard]] Entity at(uint32_t index) const {
            if(index < fragmented_.size()) {
                return fragmented_[index];
            }
            if(index - fragmented_.size() < count_) {
                return Entity::makeFromValue(first_ + index);
            }
            throw std::out_of_range("Invalid index");
        }
        Entity operator[](uint32_t index) const noexcept {
            if(index < fragmented_.size()) {
                return fragmented_[index];
            }
            return Entity::makeFromValue(first_ + index);
        }

        [[nodiscard]] Iterator begin() const;
        [[nodiscard]] Iterator end() const;

        [[nodiscard]] uint32_t numFragmented() const noexcept {
            return static_cast<uint32_t>(fragmented_.size());
        }
        [[nodiscard]] uint32_t size() const noexcept {
            return count_ + numFragmented();
        }
    private:
        mustache::vector<Entity> fragmented_;
        uint32_t first_{0u};
        uint32_t count_{0u};
    };
}
