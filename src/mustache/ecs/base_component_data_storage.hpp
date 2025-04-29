#pragma once

#include <mustache/utils/default_settings.hpp>

#include <mustache/ecs/id_deff.hpp>

#include <memory>

namespace mustache {

    class MUSTACHE_EXPORT BaseComponentDataStorage {
    public:
        virtual ~BaseComponentDataStorage() = default;

        [[nodiscard]] virtual uint32_t capacity() const noexcept = 0;

        virtual void reserve(size_t new_capacity) = 0;
        virtual void clear(bool free_chunks = true) = 0;

        virtual void* getDataSafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept = 0;
        virtual void* getDataUnsafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept = 0;
        [[nodiscard]] virtual uint32_t distToChunkEnd(ComponentStorageIndex index) const noexcept = 0;

        virtual void emplace(ComponentStorageIndex position);

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void* getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                return getDataSafe(component_index, index);
            } else {
                return getDataUnsafe(component_index, index);
            }
        }

        [[nodiscard]] MUSTACHE_INLINE ComponentStorageIndex lastItemIndex() const noexcept {
            return ComponentStorageIndex::make(size_ - 1);
        }

        [[nodiscard]] MUSTACHE_INLINE uint32_t size() const noexcept {
            return size_;
        }

        [[nodiscard]] MUSTACHE_INLINE bool isEmpty() const noexcept {
            return size_ == 0u;
        }

        MUSTACHE_INLINE virtual void incSize() noexcept {
            ++size_;
        }

        MUSTACHE_INLINE virtual void decrSize() noexcept {
            --size_;
        }

    protected:
        uint32_t size_{0u};
    };
}
