#pragma once

#include <mustache/ecs/world.hpp>

namespace mustache {
/**
     * Stores result of archetype filtering
     */
    struct DefaultWorldFilterResult {
        struct ArchetypeFilterResult {
            Archetype* archetype {nullptr};
            uint32_t entities_count {0};
            std::vector<ChunkIndex> chunks;
        };
        void apply(World& world);
        std::vector<ArchetypeFilterResult> filtered_archetypes;
        ComponentMask mask_;
        uint32_t total_entity_count{0u};
    };

    template<typename _Filter>
    struct CustomWorldFilterResult {
        template<typename>
        bool isArchetypeMatchGenerated(Archetype& archetype, ...) { // default implementationZ
            return archetype.isMatch(mask_);
        }
        template<typename _DerivedFilterType> // this function will be called if derived class has isArchetypeMatch function
        bool isArchetypeMatchGenerated(Archetype& archetype, decltype(&_DerivedFilterType::isArchetypeMatch)) {
            _Filter& self = *static_cast<_Filter*>(this);
            return self.isArchetypeMatch(archetype);
        }

        template<typename _DerivedFilterType>
        static constexpr bool hasPerChunkFilter(decltype(&_DerivedFilterType::isChunkMatch)) noexcept {
            return true;
        }

        template<typename>
        static constexpr bool hasPerChunkFilter(...) noexcept {
            return false;
        }

        struct ArchetypeFilterResult {
            Archetype* archetype {nullptr};
            uint32_t entities_count {0};
            std::vector<ChunkIndex> chunks;
        };

        void apply(World& world) {
            filtered_archetypes.clear();
            total_entity_count = 0;

            auto& entities = world.entities();
            const auto num_archetypes = entities.getArchetypesCount();
            filtered_archetypes.reserve(num_archetypes);

            for(ArchetypeIndex index = ArchetypeIndex::make(0); index < ArchetypeIndex::make(num_archetypes); ++index) {
                auto& arch = entities.getArchetype(index);
                if(arch.size() > 0u && arch.isMatch(mask_)) {
                    ArchetypeFilterResult item;
                    if constexpr (hasPerChunkFilter<_Filter>(nullptr)) {
                        auto& self = *static_cast<_Filter*>(this);

                        const auto chunk_count = arch.chunkCount();
                        const auto begin = ChunkIndex::make(0);
                        const auto end = ChunkIndex::make(chunk_count);
                        const auto last_chunk_index = chunk_count > 0 ?
                                ChunkIndex::make(chunk_count - 1) :
                                ChunkIndex::null();
                        const auto chunk_capacity = arch.chunkCapacity();
                        for (auto chunk_index = begin; chunk_index < end; ++chunk_index) {
                            if (self.isChunkMatch(arch, chunk_index)) {
                                const auto entities_in_chunk = chunk_index != last_chunk_index ?
                                        chunk_capacity :
                                        arch.size() % chunk_capacity;
                                item.entities_count += entities_in_chunk;
                                item.chunks.push_back(chunk_index);
                            }
                        }
                    } else {
                        item.entities_count = arch.size();
                    }
                    if (item.entities_count > 0) {
                        total_entity_count += item.entities_count;
                        item.archetype = &arch;
                        filtered_archetypes.push_back(item);
                    }
                }
            }
        }

        struct TaskView {
            JobInvocationIndex invocation_index;
            TaskArchetypeIndex cur_archetype;
            ArchetypeEntityIndex first_entity;
            uint32_t task_size;
            struct End {

            };
            struct Iterator {
                struct Data {
                    Archetype* archetype;
                    ArchetypeEntityIndex first_entity;
                    uint32_t count;
                };
                Iterator& operator++() noexcept {
                    /// TODO: impl me
                    return *this;
                }
                Data operator*() const noexcept {
                    /// TODO: impl me
                    return {};
                }
            };

        };

        std::vector<ArchetypeFilterResult> filtered_archetypes;
        ComponentMask mask_;
        uint32_t total_entity_count{0u};
    };

}
