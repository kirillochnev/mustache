#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/utils/array_wrapper.hpp>

#include <array>

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
        MUSTACHE_INLINE void clear(bool free_chunks = true);

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE Entity* getEntityData(ComponentStorageIndex index) const noexcept;
        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE void* getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept;
        MUSTACHE_INLINE ElementView getElementView(ComponentStorageIndex index) const noexcept;

        template<FunctionSafety _Safety = FunctionSafety::kDefault>
        MUSTACHE_INLINE WorldVersion getVersion(ComponentIndex component_index, ComponentStorageIndex index) const noexcept;
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

        class Chunk;

        [[nodiscard]] MUSTACHE_INLINE uint32_t componentsCount() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE ComponentOffset entityOffset() const noexcept;
        [[nodiscard]] MUSTACHE_INLINE ChunkCapacity chunkCapacity() const noexcept;
        MUSTACHE_INLINE void reserveForNextItem();

        void allocateChunk();
        void freeChunk(Chunk* chunk) noexcept;

        ArrayWrapper<std::vector<ComponentDataGetter>, ComponentIndex> component_getter_info_; // ComponentIndex -> {offset, size}
        uint32_t size_{0u};
        ChunkCapacity chunk_capacity_;
        ArrayWrapper<std::vector<Chunk*>, ChunkIndex> chunks_;
    };

    class ComponentDataStorage::Chunk {
    public:
        enum : uint32_t {
            kChunkSize = 1024 * 1024
        };
        Chunk() = default;
        Chunk(const Chunk&) = delete;
        template <typename T = std::byte>
        [[nodiscard]] T* data() noexcept {
            return reinterpret_cast<T*>(data_.data());
        }
        template <typename T = std::byte>
        [[nodiscard]] const T* data() const noexcept {
            return reinterpret_cast<const T*>(data_.data());
        }
        template <typename T = void>
        [[nodiscard]] T* dataPointerWithOffset(ComponentOffset offset) noexcept {
            return reinterpret_cast<T*>(offset.apply(data_.data()));
        }
        template <typename T = void>
        [[nodiscard]] const T* dataPointerWithOffset(ComponentOffset offset) const noexcept {
            return reinterpret_cast<const T*>(offset.apply(data_.data()));
        }

    private:
        std::array<std::byte, kChunkSize> data_;
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
        auto version_array = chunk->data<WorldVersion>();
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
        auto version_array = chunk->data<WorldVersion>();
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

    template<FunctionSafety _Safety>
    WorldVersion ComponentDataStorage::getVersion(ComponentIndex component, ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (component.isNull() || component.toInt() >= componentsCount() || size_ <= index.toInt()) {
                return WorldVersion::null();
            }
        }
        const auto chunk = chunks_[index / chunkCapacity()];
        return chunk->data<WorldVersion>()[component.toInt()];
    }

    class ComponentDataStorage::ElementView {
    public:

        [[nodiscard]] ComponentStorageIndex globalIndex() const noexcept {
            return ComponentStorageIndex::make(
                chunk_index_.toInt() * storage_->chunkCapacity().toInt() + entity_index_.toInt()
            );
        }

        [[nodiscard]] uint32_t elementArraySize() const noexcept {
            const auto diff = [](const auto a, const auto b) noexcept{
                return b > a ? b - a : 0;
            };
            const auto global_index = globalIndex();
            const auto storage_size = storage_->size();
            const auto elements_in_chunk = diff(entity_index_.toInt(), storage_->chunkCapacity().toInt());
            const auto elements_in_arch = diff(global_index.toInt(), storage_size);
            return std::min(elements_in_arch, elements_in_chunk);
        }

        [[nodiscard]] bool isValid() const noexcept {
            return storage_ && storage_->chunks_.has(chunk_index_) &&
                storage_->chunkCapacity().isIndexValid(entity_index_) &&
                    globalIndex() <= storage_->lastItemIndex();
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        Entity* getEntity() noexcept {
            if constexpr (isSafe(_Safety)) {
                if (!isValid()) {
                    return nullptr;
                }
            }
            auto entity_ptr = storage_->chunks_[chunk_index_]->dataPointerWithOffset<Entity>(storage_->entityOffset());
            return entity_ptr + entity_index_.toInt();
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        void* getData(ComponentIndex component_index) const noexcept {
            if constexpr (isSafe(_Safety)) {
                if (component_index.isNull() || !isValid()) {
                    return nullptr;
                }
            }
            const auto& info = storage_->component_getter_info_[component_index];
            const auto offset = info.offset.add(info.size * entity_index_.toInt());
            auto chunk = storage_->chunks_[chunk_index_];
            return chunk->dataPointerWithOffset(offset);
        }

        ElementView& operator+=(uint32_t count) noexcept {
            const auto new_index = entity_index_.toInt() + count;
            chunk_index_ = ChunkIndex::make(chunk_index_.toInt() + new_index / storage_->chunkCapacity().toInt());
            entity_index_ = ChunkEntityIndex::make(new_index % storage_->chunkCapacity().toInt());
            return *this;
        }

        ElementView& operator++() noexcept {
            return (*this) += 1;
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
