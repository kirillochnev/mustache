#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/chunk.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/utils/array_wrapper.hpp>

namespace mustache {
    class ComponentDataStorage {
    public:
        class ElementView;

        explicit ComponentDataStorage(const ComponentMask& mask);

        [[nodiscard]] MUSTACHE_INLINE uint32_t capacity() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE uint32_t size() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE bool isEmpty() const noexcept;

        MUSTACHE_INLINE void reserve(size_t new_capacity);
        MUSTACHE_INLINE void incSize() noexcept;
        MUSTACHE_INLINE void decrSize() noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE Entity* getEntityData(ComponentStorageIndex index) const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void* getData(ComponentIndex component_index, Chunk* chunk, ChunkEntityIndex index) const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void* getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept;

        MUSTACHE_INLINE ElementView getElementView(ComponentStorageIndex index) const noexcept;

        MUSTACHE_INLINE void clear(bool free_chunks = true);

        MUSTACHE_INLINE void reserveForNextItem();

        [[nodiscard]] MUSTACHE_INLINE uint32_t componentsCount() const noexcept;

        [[nodiscard]] MUSTACHE_INLINE ComponentOffset entityOffset() const noexcept;

        [[nodiscard]] MUSTACHE_INLINE ChunkCapacity chunkCapacity() const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void updateComponentVersion(WorldVersion version, ComponentIndex component_index, ComponentStorageIndex index) noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void updateVersion(WorldVersion version, ComponentStorageIndex index) noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE ComponentStorageIndex pushBackAndUpdateVersion(Entity entity, WorldVersion world_version);

        [[nodiscard]] MUSTACHE_INLINE ComponentStorageIndex lastItemIndex() const noexcept;
    private:

        struct ComponentDataGetter {
            ComponentOffset offset;
            uint32_t size;
        };

        friend class Archetype;
        void allocateChunk();
        void freeChunk(Chunk* chunk) noexcept;

        ArrayWrapper<std::vector<ComponentDataGetter>, ComponentIndex> component_getter_info_; // ComponentIndex -> {offset, size}
        uint32_t size_{0u};
        ChunkCapacity chunk_capacity_;
        ArrayWrapper<std::vector<Chunk*>, ChunkIndex> chunks_;
    };


    uint32_t ComponentDataStorage::capacity() const noexcept {
        return chunk_capacity_.toInt() * chunks_.size();
    }

    uint32_t ComponentDataStorage::size() const noexcept {
        return size_;
    }

    bool ComponentDataStorage::isEmpty() const noexcept {
        return size_ == 0u;
    }

    void ComponentDataStorage::reserve(size_t new_capacity) {
        while (capacity() < new_capacity) {
            allocateChunk();
        }
    }

    void ComponentDataStorage::incSize() noexcept {
        ++size_;
    }

    void ComponentDataStorage::decrSize() noexcept {
        --size_;
    }

    template<FunctionSafety _Safety>
    Entity* ComponentDataStorage::getEntityData(ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (index.isNull() || index.toInt() >= size_) {
                return nullptr;
            }
        }
        const auto index_at_chunk = index % chunk_capacity_;
        const auto read_offset = entityOffset().add(sizeof(Entity) * index_at_chunk.toInt());
        return chunks_[index / chunk_capacity_]->dataPointerWithOffset<Entity>(read_offset);
    }
    template<FunctionSafety _Safety>
    void* ComponentDataStorage::getData(ComponentIndex component_index, Chunk* chunk, ChunkEntityIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (component_index.isNull() || !index.isValid() || chunk == nullptr ||
                !component_getter_info_.has(component_index) /*|| index.toInt() >= size_*/ ||
                chunk_capacity_.toInt() <= index.toInt()) {
                return nullptr;
            }
        }
        const auto& info = component_getter_info_[component_index];
        const auto offset = info.offset.add(info.size * index.toInt());
        return chunk->dataPointerWithOffset(offset);
    }

    template<FunctionSafety _Safety>
    void* ComponentDataStorage::getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (component_index.isNull() || index.isNull() ||
                !component_getter_info_.has(component_index) || index.toInt() >= size_) {
                return nullptr;
            }
        }
        const auto chunk_index = index % chunk_capacity_;
        const auto& info = component_getter_info_[component_index];
        const auto offset = info.offset.add(info.size * chunk_index.toInt());
        auto chunk = chunks_[index / chunk_capacity_];
        return chunk->dataPointerWithOffset(offset);
    }

    void ComponentDataStorage::clear(bool free_chunks) {
        if (free_chunks) {
            for (auto chunk : chunks_) {
                freeChunk(chunk);
            }
            chunks_.clear();
        }
        size_ = 0;
    }

    void ComponentDataStorage::reserveForNextItem() {
        reserve(size_ + 1);
    }

    uint32_t ComponentDataStorage::componentsCount() const noexcept {
        return component_getter_info_.size();
    }

    ComponentOffset ComponentDataStorage::entityOffset() const noexcept {
        return ComponentOffset::makeAligned(ComponentOffset::make(sizeof(WorldId) * componentsCount()),
                                            alignof(Entity));
    }

    ChunkCapacity ComponentDataStorage::chunkCapacity() const noexcept {
        return chunk_capacity_;
    }

    template<FunctionSafety _Safety>
    void ComponentDataStorage::updateComponentVersion(WorldVersion version, ComponentIndex component_index,
                                                ComponentStorageIndex index) noexcept {
        if constexpr (isSafe(_Safety)) {
            if (!component_getter_info_.has(component_index) || index.toInt() >= size_) {
                return;
            }
        }
        auto& chunk = chunks_[index / chunk_capacity_];
        auto version_array = chunk->dataPointerWithOffset<WorldVersion>(ComponentOffset::make(0));
        version_array[component_index.toInt()] = version;
    }
    template<FunctionSafety _Safety>
    void ComponentDataStorage::updateVersion(WorldVersion version, ComponentStorageIndex index) noexcept {
        if constexpr (isSafe(_Safety)) {
            if (index.toInt() >= size_) {
                return;
            }
        }
        auto& chunk = chunks_[index / chunk_capacity_];
        auto version_array = chunk->dataPointerWithOffset<WorldVersion>(ComponentOffset::make(0));
        for (uint32_t i = 0; i < componentsCount(); ++i) {
            version_array[i] = version;
        }
    }

    template<FunctionSafety _Safety>
    ComponentStorageIndex ComponentDataStorage::pushBackAndUpdateVersion(Entity entity, WorldVersion world_version) {
        reserveForNextItem();
        ComponentStorageIndex index = ComponentStorageIndex::make(size_);
        incSize();
        *getEntityData<_Safety>(index) = entity;
        updateVersion(world_version, index);
        return index;
    }

    ComponentStorageIndex ComponentDataStorage::lastItemIndex() const noexcept {
        return ComponentStorageIndex::make(size_ - 1);
    }

    class ComponentDataStorage::ElementView {
    public:
        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        void* getData(ComponentIndex component_index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (component_index.isNull() ||
                    !storage_->chunks_.has(chunk_index_) ||
                    storage_->chunkCapacity().toInt() <= entity_index_.toInt()) {
                    return nullptr;
                }
            }
            const auto& info = storage_->component_getter_info_[component_index];
            const auto offset = info.offset.add(info.size * entity_index_.toInt());
            auto chunk = storage_->chunks_[chunk_index_];
            return chunk->dataPointerWithOffset(offset);
        }

    private:
        friend ComponentDataStorage;
        const ComponentDataStorage* storage_;
        ChunkIndex chunk_index_;
        ChunkEntityIndex entity_index_;
        constexpr ElementView(const ComponentDataStorage* storage,
                                  ChunkIndex chunk_index, ChunkEntityIndex entity_index):
                storage_{storage},
                chunk_index_{chunk_index},
                entity_index_{entity_index} {

        }
    };

    ComponentDataStorage::ElementView ComponentDataStorage::getElementView(ComponentStorageIndex index) const noexcept {
        return ElementView{this, index / chunkCapacity(), index % chunkCapacity()};
    }
}
