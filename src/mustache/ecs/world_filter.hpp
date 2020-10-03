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
        ComponentIdMask mask_;
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
                    // TODO: impl me
                    /*if constexpr (hasPerChunkFilter<_Filter>(nullptr)) {
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
                    } else*/ {
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

        std::vector<ArchetypeFilterResult> filtered_archetypes;
        ComponentIdMask mask_;
        uint32_t total_entity_count{0u};
    };

    struct TaskView {
        TaskView(DefaultWorldFilterResult& filter_result, uint32_t num_tasks):
            begin_{filter_result, num_tasks},
            end_ {PerEntityJobTaskId::make(num_tasks)} {

        }
        struct End {
            PerEntityJobTaskId task_id_;
        };

        class Iterator {
        public:
            struct TaskInfo {
                uint32_t current_task_size;
                PerEntityJobTaskId task_id = PerEntityJobTaskId::make(0u);
                TaskArchetypeIndex first_archetype = TaskArchetypeIndex::make(0u);
                ArchetypeEntityIndex first_entity = ArchetypeEntityIndex::make(0u);
            };

            bool operator!=(const End& end) const noexcept {
                return task_info_.task_id.isValid() && task_info_.task_id != end.task_id_;
            }

            void updateTaskSize() noexcept {
                task_info_.current_task_size = task_info_.task_id.toInt() < tasks_with_extra_item_ ? ept_ + 1 : ept_;
            }

            Iterator& operator++() noexcept {
                uint32_t num_entities_to_iterate = task_info_.current_task_size;
                while (num_entities_to_iterate != 0) {
                    const auto& archetype_info = filter_result_->filtered_archetypes[task_info_.first_archetype.toInt()];
                    const uint32_t num_free_entities_in_arch = archetype_info.entities_count - task_info_.first_entity.toInt();
                    if (num_free_entities_in_arch > num_entities_to_iterate) {
                        task_info_.first_entity = ArchetypeEntityIndex::make(task_info_.first_entity.toInt() + num_entities_to_iterate);
                        num_entities_to_iterate = 0u;
                    } else {
                        num_entities_to_iterate -= num_free_entities_in_arch;
                        ++task_info_.first_archetype;
                        task_info_.first_entity = ArchetypeEntityIndex::make(0);
                    }
                }
                ++task_info_.task_id;
                updateTaskSize();
                return *this;
            }
            const TaskInfo& operator*() const noexcept {
                return task_info_;
            }

        private:
            friend TaskView;
            Iterator(DefaultWorldFilterResult& filter_result, uint32_t num_tasks):
                    filter_result_{&filter_result},
                    ept_{filter_result.total_entity_count / num_tasks},
                    tasks_with_extra_item_ {filter_result.total_entity_count - num_tasks * ept_} {
                updateTaskSize();
            }
            DefaultWorldFilterResult* filter_result_;
            uint32_t ept_;
            uint32_t tasks_with_extra_item_;
            TaskInfo task_info_;
        };

        Iterator begin() {
            return begin_;
        }
        End end() {
            return end_;
        }
    private:
        Iterator begin_;
        End end_;
    };
}
