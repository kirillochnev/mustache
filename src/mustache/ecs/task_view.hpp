#pragma once

#include <mustache/ecs/world_filter.hpp>
#include <mustache/ecs/archetype.hpp>

namespace mustache {

    struct TaskInfo {
        uint32_t size;
        ParallelTaskId id = ParallelTaskId::make(0u);
        TaskArchetypeIndex first_archetype = TaskArchetypeIndex::make(0u);
        ArchetypeEntityIndex first_entity = ArchetypeEntityIndex::make(0u);
    };

    struct ArrayView : private ElementView {
        using FilrerResult = WorldFilterResult::ArchetypeFilterResult;



        void updateBlock() {
            if (dist_to_end_ > 0) {
                const auto index_in_archetype = globalIndex().toInt();

                dist_to_block_end_ = filter_result_->blocks[current_block_].end.toInt() - index_in_archetype;
                if (dist_to_block_end_ == 0u) {
                    ++current_block_;
                    const auto& block = filter_result_->blocks[current_block_];
                    *this += block.begin.toInt() - index_in_archetype;
                    dist_to_block_end_ = block.end.toInt() - block.begin.toInt();
                }
                array_size_ = std::min(dist_to_block_end_, std::min(distToChunkEnd(), dist_to_end_));
            }
        }

        ArrayView(WorldFilterResult& filter_result, TaskArchetypeIndex archetype_index,
                  ArchetypeEntityIndex first_entity, uint32_t size, WorldFilterResult::BlockIndex block_index):
                ElementView{filter_result.filtered_archetypes[archetype_index.toInt()].archetype->getElementView(first_entity)},
                current_block_{block_index},
                dist_to_end_ {std::min(size, archetype_->size() - first_entity.toInt())},
                filter_result_ {&filter_result.filtered_archetypes[archetype_index.toInt()]} {
            updateBlock();
        }

        ArrayView& begin() { return *this; }
        std::nullptr_t end() const noexcept { return nullptr; }
        bool operator != (std::nullptr_t) const noexcept { return dist_to_end_ != 0u; }
        const ArrayView& operator*() const noexcept { return *this; }

        ArrayView& operator++() noexcept {
            dist_to_end_ -= array_size_;
            *this += array_size_;
            updateBlock();
            return *this;
        }

        using ElementView::getData;
        using ElementView::getEntity;
        Archetype* archetype() const noexcept {
            return filter_result_->archetype;
        }
        [[nodiscard]] uint32_t arraySize() const noexcept {
            return array_size_;
        }

        static ArrayView make(WorldFilterResult& filter_result, TaskArchetypeIndex archetype_index,
                              ArchetypeEntityIndex first_entity, uint32_t size) noexcept {
            int32_t count = first_entity.toInt<int32_t >();
            auto block_index = WorldFilterResult::BlockIndex::make(0u);
            while (count > 0) {
                const auto block = filter_result.filtered_archetypes[archetype_index.toInt()].blocks[block_index];
                const int32_t block_size = block.end.toInt<int32_t >() - block.begin.toInt<int32_t >();
                if (count > block_size) {
                    count -= block_size;
                    ++block_index;
                } else {
                    break;
                }
            }
            const auto block_begin = filter_result.filtered_archetypes[archetype_index.toInt()].blocks[block_index].begin;
            first_entity = ArchetypeEntityIndex::make(block_begin.toInt<int32_t >() + count);
            return ArrayView{filter_result, archetype_index, first_entity, size, block_index};
        }
    private:
        WorldFilterResult::BlockIndex current_block_ = WorldFilterResult::BlockIndex::make(0u);
        uint32_t array_size_ = 0u;
        uint32_t dist_to_end_ = 0u;
        FilrerResult* filter_result_ = nullptr;
        uint32_t dist_to_block_end_ = 0;
    };

    struct ArchetypeGroup {
        ArchetypeGroup(const TaskInfo& info, WorldFilterResult& fr):
                dist_to_end{info.size},
                archetype_index{info.first_archetype},
                filtered_archetypes{&fr.filtered_archetypes},
                first_entity{info.first_entity} {
            if (!filtered_archetypes->empty()) {
                const auto& archetype_info = (*filtered_archetypes)[archetype_index.toInt()];
                const uint32_t num_free_entities_in_arch = archetype_info.entities_count - info.first_entity.toInt();
                current_size = std::min(dist_to_end, num_free_entities_in_arch);
            }
        }
        void incrementArchetype() {
            dist_to_end -= current_size;
            if (dist_to_end > 0) {
                ++archetype_index;
                const auto& archetype_info = (*filtered_archetypes)[archetype_index.toInt()];
                const uint32_t num_free_entities_in_arch = archetype_info.entities_count;
                current_size = std::min(dist_to_end, num_free_entities_in_arch);
                first_entity = ArchetypeEntityIndex::make(0);
            } else {
                current_size = 0;
            }
        }
        [[nodiscard]]  uint32_t taskSize() const noexcept {
            return dist_to_end;
        }

        ArchetypeGroup& operator++() noexcept {
            incrementArchetype();
            return *this;
        }

        Archetype* archetype() const noexcept {
            return (*filtered_archetypes)[archetype_index.toInt()].archetype;
        }

        const ArchetypeGroup& operator*() const noexcept { return *this; }
        bool operator != (std::nullptr_t) const noexcept {  return dist_to_end != 0u; }
        ArchetypeGroup begin() const noexcept { return *this; }
        std::nullptr_t end() const noexcept { return nullptr; }

        uint32_t current_size {0u};
        uint32_t dist_to_end {0u};
        TaskArchetypeIndex archetype_index = TaskArchetypeIndex::make(0u);
        decltype(WorldFilterResult::filtered_archetypes)* filtered_archetypes = nullptr;
        ArchetypeEntityIndex first_entity = ArchetypeEntityIndex::make(0u);
    };

    struct TaskGroup {
    public:
        struct End {
            ParallelTaskId task_id_;
        };

        bool operator!=(const End& end) const noexcept {
            return task_info_.id.isValid() && task_info_.id != end.task_id_;
        }

        void updateTaskSize() noexcept {
            task_info_.size = task_info_.id.toInt() < tasks_with_extra_item_ ? ept_ + 1 : ept_;
        }

        TaskGroup& operator++() noexcept {
            uint32_t num_entities_to_iterate = task_info_.size;
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
            ++task_info_.id;
            updateTaskSize();
            return *this;
        }

        ArchetypeGroup operator*() const noexcept {
            return ArchetypeGroup{task_info_, *filter_result_};
        }

        TaskGroup(WorldFilterResult& filter_result, uint32_t num_tasks):
                filter_result_{&filter_result},
                ept_{filter_result.total_entity_count / num_tasks},
                tasks_with_extra_item_ {filter_result.total_entity_count - num_tasks * ept_} {
            updateTaskSize();
        }

        static auto make(WorldFilterResult& filter_result, uint32_t num_tasks) noexcept {
            struct Result {
                Result(WorldFilterResult &filter_result, uint32_t num_tasks) :
                        begin_{filter_result, num_tasks},
                        end_{ParallelTaskId::make(num_tasks)} {

                }

                TaskGroup begin() noexcept {
                    return begin_;
                }

                TaskGroup::End end() const noexcept {
                    return end_;
                }

            private:
                TaskGroup begin_;
                TaskGroup::End end_;
            };

            return Result{filter_result, num_tasks};
        }
    private:
        WorldFilterResult* filter_result_;
        uint32_t ept_;
        uint32_t tasks_with_extra_item_;
        TaskInfo task_info_;
    };
}
