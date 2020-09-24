#pragma once

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/utils/array_wrapper.hpp>

#include <array>
#include <iostream>

namespace mustache {
    class NewComponentDataStorage {
    public:
        class ElementView;

        explicit NewComponentDataStorage(const ComponentMask& mask);

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
        MUSTACHE_INLINE void allocateBlock();
        enum : uint32_t {
            kComponentBlockSize = 1024
        };
        static constexpr ChunkCapacity chunkCapacity() noexcept {
            return ChunkCapacity::make(kComponentBlockSize);
        }
        struct ComponentDataHolder {
            ~ComponentDataHolder() {
                clear();
            }
            void clear() {
                for (auto ptr : data) {
                    free(ptr);
                }
            }
            void allocate() {
                auto ptr = aligned_alloc(component_alignment, component_size * kComponentBlockSize);
                data.push_back(static_cast<std::byte*>(ptr));
            }
            void reserve(uint32_t new_capacity) {
                while (data.size() * kComponentBlockSize < new_capacity) {
                    allocate();
                }
            }

            template<FunctionSafety _Safety = FunctionSafety::kDefault>
            void* get(ComponentStorageIndex index) const noexcept {
                if constexpr (isSafe(_Safety)) {
                    if (index.toInt() >= kComponentBlockSize * data.size()) {
                        return nullptr;
                    }
                }
                constexpr auto chunk_capacity = chunkCapacity();//ChunkCapacity::make(kComponentBlockSize);
                auto block = data[(index / chunk_capacity).toInt()];
                return block + component_size * (index % chunk_capacity).toInt();
            }
            std::vector<std::byte*> data;
            uint32_t component_size;
            uint32_t component_alignment;
        };
        uint32_t size_ = 0;
        uint32_t capacity_ = 0;
        std::vector<WorldVersion> versions_;
        ArrayWrapper<std::vector<Entity>, ComponentStorageIndex> entities_;
        ArrayWrapper<std::vector<ComponentDataHolder>, ComponentIndex> components_;
    };

    uint32_t NewComponentDataStorage::capacity() const noexcept {
        return capacity_;
    }

    uint32_t NewComponentDataStorage::size() const noexcept {
        return size_;
    }

    bool NewComponentDataStorage::isEmpty() const noexcept {
        return entities_.empty();
    }
    void NewComponentDataStorage::allocateBlock() {
        for (auto& component : components_) {
            component.allocate();
        }
        capacity_ += kComponentBlockSize;
    }

    void NewComponentDataStorage::reserve(size_t new_capacity) {
        if (capacity_ >= new_capacity) {
            return;
        }
        entities_.reserve(new_capacity);
        while (capacity_ < new_capacity) {
            allocateBlock();
        }
    }

    void NewComponentDataStorage::incSize() noexcept {
        ++size_;
        entities_.emplace_back();
    }

    void NewComponentDataStorage::decrSize() noexcept {
        --size_;
        entities_.pop_back();
    }

    void NewComponentDataStorage::clear(bool free_chunks) {
        entities_.clear();
        size_ = 0;
        if (free_chunks) {
            versions_.clear();
            components_.clear();
            capacity_ = 0;
        }
    }

    template<FunctionSafety _Safety>
    Entity* NewComponentDataStorage::getEntityData(ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (!entities_.has(index)) {
                return nullptr;
            }
        }
        return const_cast<Entity*>(&entities_[index]);
    }

    template<FunctionSafety _Safety>
    void* NewComponentDataStorage::getData(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (!entities_.has(index) || !components_.has(component_index)) {
                return nullptr;
            }
        }
        return components_[component_index].get(index);
    }

    template<FunctionSafety _Safety>
    WorldVersion
    NewComponentDataStorage::getVersion(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (!entities_.has(index) || !components_.has(component_index)) {
                return WorldVersion::null();
            }
        }
        /*constexpr auto chunk_capacity = ChunkCapacity::make(kComponentBlockSize);
        const auto block_index = index / chunk_capacity;
        versions_[]*/
        return WorldVersion();
    }

    template<FunctionSafety _Safety>
    void NewComponentDataStorage::updateComponentVersion(WorldVersion version, ComponentIndex component_index,
                                                         ComponentStorageIndex index) noexcept {
        (void) version;
        (void ) component_index;
        (void) index;
    }

    template<FunctionSafety _Safety>
    void NewComponentDataStorage::updateVersion(WorldVersion version, ComponentStorageIndex index) noexcept {
        (void) version;
        (void) index;
    }

    template<FunctionSafety _Safety>
    ComponentStorageIndex NewComponentDataStorage::pushBackAndUpdateVersion(Entity entity, WorldVersion world_version) {
        const auto cur_size = size();
        const auto new_size = cur_size + 1;
        reserve(new_size);
        ComponentStorageIndex index = ComponentStorageIndex::make(cur_size);
        incSize();
        *getEntityData<_Safety>(index) = entity;
        updateVersion(world_version, index);
        return index;
    }

    ComponentStorageIndex NewComponentDataStorage::lastItemIndex() const noexcept {
        return ComponentStorageIndex::make(size() - 1);
    }

    class NewComponentDataStorage::ElementView { // TODO: rename
    public:

        [[nodiscard]] ComponentStorageIndex globalIndex() const noexcept {
            return ComponentStorageIndex::make(
                    chunk_index_.toInt() * chunkCapacity().toInt() + entity_index_.toInt()
            );
        }

        [[nodiscard]] uint32_t elementArraySize() const noexcept {
            const auto diff = [](const auto a, const auto b) noexcept{
                return b > a ? b - a : 0;
            };
            const auto global_index = globalIndex();
            const auto storage_size = storage_->size();
            const auto elements_in_chunk = diff(entity_index_.toInt(), chunkCapacity().toInt());
            const auto elements_in_arch = diff(global_index.toInt(), storage_size);
            return std::min(elements_in_arch, elements_in_chunk);
        }

        [[nodiscard]] bool isValid() const noexcept {
            return globalIndex() <= storage_->lastItemIndex();
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        Entity* getEntity() noexcept {
            return storage_->getEntityData<_Safety>(globalIndex());
        }

        template<FunctionSafety _Safety = FunctionSafety::kSafe>
        void* getData(ComponentIndex component_index) const noexcept {
            return storage_->getData<_Safety>(component_index, globalIndex());
        }

        ElementView& operator+=(uint32_t count) noexcept {
            const auto new_index = entity_index_.toInt() + count;
            chunk_index_ = ChunkIndex::make(chunk_index_.toInt() + new_index / chunkCapacity().toInt());
            entity_index_ = ChunkEntityIndex::make(new_index % chunkCapacity().toInt());
            return *this;
        }

        ElementView& operator++() noexcept {
            return (*this) += 1;
        }
    private:
        friend NewComponentDataStorage;
        const NewComponentDataStorage* storage_;
        ChunkIndex chunk_index_;
        ChunkEntityIndex entity_index_;
        constexpr ElementView(const NewComponentDataStorage* storage,
                              ChunkIndex chunk_index, ChunkEntityIndex entity_index):
                storage_{storage},
                chunk_index_{chunk_index},
                entity_index_{entity_index} {

        }
    };

    NewComponentDataStorage::ElementView NewComponentDataStorage::getElementView(ComponentStorageIndex index) const noexcept {
        (void ) index;
        return ElementView {this, index / chunkCapacity(), index % chunkCapacity()};
    }
}
