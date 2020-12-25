#include "new_component_data_storage.hpp"
#include "component_factory.hpp"
#include <mustache/utils/logger.hpp>

using namespace mustache;

namespace {
    enum : uint32_t {
        kComponentBlockSize = 1024 * 16
    };
}

struct NewComponentDataStorage::ComponentDataHolder {
    ~ComponentDataHolder() {
        clear();
    }
    explicit ComponentDataHolder(MemoryManager& manager):
            memory_manager{&manager},
            data{manager} {

    }

    void clear() {
        for (auto ptr : data) {
            memory_manager->deallocate(ptr);
        }
    }
    void allocate() {
        auto* ptr = memory_manager->allocate(component_size * kComponentBlockSize, component_alignment);
        data.push_back(static_cast<std::byte*>(ptr));
    }

    template<FunctionSafety _Safety = FunctionSafety::kDefault>
    void* get(ComponentStorageIndex index) const noexcept {
        if constexpr (isSafe(_Safety)) {
            if (index.toInt() >= kComponentBlockSize * data.size()) {
                return nullptr;
            }
        }
        const auto chunk_capacity = chunkCapacity();
        auto block = data[(index / chunk_capacity).toInt()];
        return block + component_size * (index % chunk_capacity).toInt();
    }
    MemoryManager* memory_manager = nullptr;
    std::vector<std::byte*, Allocator<std::byte*> > data;
    uint32_t component_size;
    uint32_t component_alignment;
};

NewComponentDataStorage::NewComponentDataStorage(const ComponentIdMask& mask, MemoryManager& memory_manager):
    components_{memory_manager},
    memory_manager_{&memory_manager} {
    components_.reserve(mask.componentsCount());

    ComponentIndex index = ComponentIndex::make(0);
    mask.forEachItem([&index, this](ComponentId id) {
        const auto &info = ComponentFactory::componentInfo(id);
        auto &component = components_.emplace_back(*memory_manager_);
        component.component_size = static_cast<uint32_t>(info.size);
        component.component_alignment = static_cast<uint32_t>(info.align);
        component.memory_manager = memory_manager_;
        ++index;
    });

    Logger{}.debug("New ComponentDataStorage has been created, components: %s | chunk capacity: %d",
                   mask.toString().c_str(), chunkCapacity().toInt());
}

NewComponentDataStorage::~NewComponentDataStorage() = default;

void NewComponentDataStorage::clear(bool free_chunks) {
    size_ = 0;
    if (free_chunks) {
        components_.clear();
        capacity_ = 0;
    }
}

void NewComponentDataStorage::reserve(size_t new_capacity) {
    if (capacity_ >= new_capacity) {
        return;
    }

    while (capacity_ < new_capacity) {
        allocateBlock();
    }
}

void* NewComponentDataStorage::getDataSafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
    if (index.toInt() >= size_ || !components_.has(component_index)) {
        return nullptr;
    }
    return components_[component_index].get(index);
}

void* NewComponentDataStorage::getDataUnsafe(ComponentIndex component_index, ComponentStorageIndex index) const noexcept {
    return components_[component_index].get(index);
}

ComponentStorageIndex NewComponentDataStorage::pushBack() {
    const auto cur_size = size();
    const auto new_size = cur_size + 1;
    reserve(new_size);
    ComponentStorageIndex index = ComponentStorageIndex::make(cur_size);
    incSize();
    return index;
}

void NewComponentDataStorage::allocateBlock() {
    for (auto& component : components_) {
        component.allocate();
    }
    capacity_ += chunkCapacity().toInt();
}
ChunkCapacity NewComponentDataStorage::chunkCapacity() noexcept {
    return ChunkCapacity::make(kComponentBlockSize);
}