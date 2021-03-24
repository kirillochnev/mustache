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

#if 1
    struct ArrayView : private ElementView {
        using FilrerResult = WorldFilterResult::ArchetypeFilterResult;

        ArrayView(WorldFilterResult& filter_result, TaskArchetypeIndex archetype_index,
                  ArchetypeEntityIndex first_entity, uint32_t size):
                ElementView{filter_result.filtered_archetypes[archetype_index.toInt()].archetype->getElementView(first_entity)},
                dist_to_end_ {std::min(size, archetype_->size() - first_entity.toInt())},
                filter_result_ {&filter_result.filtered_archetypes[archetype_index.toInt()]} {
            if (dist_to_end_ > 0) {
                const auto index_in_archetype = globalIndex().toInt();

                auto dist_to_block_end = filter_result_->blocks[current_block_].end.toInt() - index_in_archetype;
                if (dist_to_block_end == 0u) {
                    ++current_block_;
                    dist_to_block_end = filter_result_->blocks[current_block_].end.toInt() - index_in_archetype;
                }
                array_size_ = std::min(dist_to_block_end, std::min(distToChunkEnd(), dist_to_end_));
            }
        }

        ArrayView& begin() { return *this; }
        std::nullptr_t end() const noexcept { return nullptr; }
        bool operator != (std::nullptr_t) const noexcept { return dist_to_end_ != 0u; }
        const ArrayView& operator*() const noexcept { return *this; }

        ArrayView& operator++() noexcept {
            dist_to_end_ -= array_size_;
            *this += array_size_;
            if (dist_to_end_ > 0) {
                const auto index_in_archetype = globalIndex().toInt();

                auto dist_to_block_end = filter_result_->blocks[current_block_].end.toInt() - index_in_archetype;
                if (dist_to_block_end == 0u) {
                    ++current_block_;
                    dist_to_block_end = filter_result_->blocks[current_block_].end.toInt() - index_in_archetype;
                }
                array_size_ = std::min(dist_to_block_end, std::min(distToChunkEnd(), dist_to_end_));
            }

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
    private:
        WorldFilterResult::BlockIndex current_block_ = WorldFilterResult::BlockIndex::make(0u);
        uint32_t array_size_ = 0u;
        uint32_t dist_to_end_ = 0u;
        FilrerResult* filter_result_ = nullptr;
    };

    struct TaskView {
        TaskView(const TaskInfo& info, WorldFilterResult& fr):
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

        TaskView& operator++() noexcept {
            incrementArchetype();
            return *this;
        }

        Archetype* archetype() const noexcept {
            return (*filtered_archetypes)[archetype_index.toInt()].archetype;
        }
        const TaskView& operator*() const noexcept { return *this; }
        bool operator != (std::nullptr_t) const noexcept {  return dist_to_end != 0u; }
        TaskView begin() const noexcept { return *this; }
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

        TaskView operator*() const noexcept {
            return TaskView{task_info_, *filter_result_};
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
#else
    class ArrayView : private ElementView {
    public:
        ArrayView(const ElementView& element_view, uint32_t entities_to_update):
                ElementView{element_view},
                entities_to_update_{entities_to_update} {

        }

        [[nodiscard]] uint32_t arraySize() const noexcept {
            return std::min(entities_to_update_, distToChunkEnd());
        }

        using ElementView::getData;
        using ElementView::getEntity;
    private:
        friend class MemoryArrayIterator;
        uint32_t entities_to_update_ = 0u;
    };

    class MemoryArrayIterator {
    public:

        MemoryArrayIterator (WorldFilterResult* filter_result, uint32_t distance_to_end,
                             TaskArchetypeIndex archetype_index, WorldFilterResult::BlockIndex block_index,
                             uint32_t position_in_block):
                            filter_result_{filter_result},
                            distance_to_end_{distance_to_end},
                            archetype_index_{archetype_index},
                            block_index_{block_index},
                            position_in_block_{position_in_block},
                            array_view_(getArrayView(filter_result, distance_to_end, archetype_index,
                                                     block_index, position_in_block)) {

        }

        ArrayView operator*() const noexcept {
            return array_view_;
        }

        MemoryArrayIterator& operator++() noexcept {
            // TODO: impl me
            distance_to_end_ -= array_view_.entities_to_update_;
            if (distance_to_end_ > 0) {
                array_view_ = getArrayView(filter_result_, distance_to_end_,
                                           archetype_index_, block_index_, position_in_block_);
            }
            return *this;
        }

        bool operator!=(std::nullptr_t) const noexcept {
            return distance_to_end_ != 0u;
        }
    private:
        static ArrayView getArrayView(WorldFilterResult* filter_result, uint32_t distance_to_end,
                                      TaskArchetypeIndex archetype_index, WorldFilterResult::BlockIndex block_index,
                                      uint32_t position_in_block) {
            const auto& archetype_info = filter_result->filtered_archetypes[archetype_index.toInt()];
            const auto block = archetype_info.blocks[block_index];
            const auto dist_to_block_end = block.block_end.toInt() - block.block_begin.toInt() - position_in_block;
            const auto entity_index = ArchetypeEntityIndex::make(block.block_begin.toInt() + position_in_block);
            const uint32_t array_size = std::min(distance_to_end, dist_to_block_end);
            return ArrayView{archetype_info.archetype->getElementView(entity_index), array_size};
        }
        friend class MemoryArraysInArchetype;
        WorldFilterResult* filter_result_ = nullptr;
        uint32_t distance_to_end_ = 0u;
        TaskArchetypeIndex archetype_index_ = TaskArchetypeIndex::make(0u);
        WorldFilterResult::BlockIndex block_index_ = WorldFilterResult::BlockIndex::make(0ull);
        uint32_t position_in_block_ = 0u;
        ArrayView array_view_;
    };

    class MemoryArraysInArchetype { // allows iterate over c-arrays of components into given task
    public:
        MemoryArrayIterator begin() const {
            return MemoryArrayIterator {
                filter_result_, distance_to_end_, archetype_index_, block_index_, position_in_block_
            };
        }

        std::nullptr_t end() const noexcept {
            return nullptr;
        }

        Archetype* archetype() const noexcept {
            return filter_result_->filtered_archetypes[archetype_index_.toInt()].archetype;
        }

    private:
        friend class ArchetypeIterator;

        WorldFilterResult* filter_result_ = nullptr;
        uint32_t distance_to_end_ = 0u;
        TaskArchetypeIndex archetype_index_ = TaskArchetypeIndex::make(0u);
        WorldFilterResult::BlockIndex block_index_ = WorldFilterResult::BlockIndex::make(0ull);
        uint32_t position_in_block_ = 0u;
    };

    class ArchetypeIterator { // allows iterate over archetypes into given task
    public:

        MemoryArraysInArchetype operator*() const noexcept {
            MemoryArraysInArchetype result;
            result.filter_result_ = filter_result_;
            result.distance_to_end_ = stepSize();
            result.archetype_index_ = archetype_index_;

            const auto& archetype_info = filter_result_->filtered_archetypes[archetype_index_.toInt()];

            uint32_t count = updated_in_archetype_;
            while (count != 0) {
                const WorldFilterResult::EntityBlock& block = archetype_info.blocks[result.block_index_];
                const auto block_size = block.block_end.toInt() - block.block_begin.toInt();
                if (block_size > count) {
                    result.position_in_block_ = count;
                    break;
                }
                count -= block_size;
                ++result.block_index_;
            }
            return result;
        }

        ArchetypeIterator& operator++() noexcept {
            const auto step_size = stepSize();

            distance_to_end_ -= step_size;

            if (distance_to_end_ != 0) {
                ++archetype_index_;
                updated_in_archetype_ = 0ull;
            }

            return *this;
        }

        bool operator!=(std::nullptr_t) const noexcept {
            return distance_to_end_ > 0;
        }

        uint32_t stepSize() const noexcept {
            const auto& archetype_info = filter_result_->filtered_archetypes[archetype_index_.toInt()];
            const auto dist_to_archetype_end = archetype_info.entities_count - updated_in_archetype_;
            const auto step_size = std::min(distance_to_end_, dist_to_archetype_end);
            return step_size;
        }
    private:
        friend class ArchetypesInTask;
        WorldFilterResult* filter_result_ = nullptr;
        uint32_t distance_to_end_ = 0u;
        TaskArchetypeIndex archetype_index_ = TaskArchetypeIndex::make(0u);
        uint32_t updated_in_archetype_ = 0ull;
    };

    class ArchetypesInTask {
    public:
        ArchetypeIterator begin() const noexcept {
            ArchetypeIterator result;
            result.filter_result_ = filter_result_;
            result.distance_to_end_ = task_size_;
            return result; // TODO: impl me
        }

        std::nullptr_t end() const noexcept {
            return nullptr;
        }
        [[nodiscard]] uint32_t taskSize() const noexcept {
            return task_size_;
        }
    private:
        friend class TaskIterator;
        WorldFilterResult* filter_result_ = nullptr;
        uint32_t task_size_ = 0ull;
    };

    using TaskView = ArchetypesInTask;


    class TaskIterator {
    public:

        TaskIterator(WorldFilterResult* filter_result, ParallelTaskId task_id, uint32_t task_count):
            filter_result_{filter_result},
            ept_{filter_result_->total_entity_count / task_count},
            tasks_with_extra_item_ {filter_result_->total_entity_count - ept_ * task_count},
            task_id_ {task_id} {

        }

        ArchetypesInTask operator*() const noexcept {
            ArchetypesInTask result;
            result.filter_result_ = filter_result_;
            result.task_size_ = taskSize();
            return result;
        }

        TaskIterator& operator++() noexcept {
            uint32_t num_entities_to_iterate = taskSize();
            while (num_entities_to_iterate != 0) {
                const auto& archetype_info = filter_result_->filtered_archetypes[archetype_index_.toInt()];
                const uint32_t num_free_entities_in_arch = archetype_info.entities_count - updated_in_archetype;
                if (num_free_entities_in_arch > num_entities_to_iterate) {
                    updated_in_archetype += num_entities_to_iterate;
                    num_entities_to_iterate = 0u;
                } else {
                    num_entities_to_iterate -= num_free_entities_in_arch;
                    ++archetype_index_;
                    updated_in_archetype = 0ull;
                }
            }
            ++task_id_;
            return *this;
        }

        bool operator!=(const ParallelTaskId& task_id) const noexcept {
            return task_id_ != task_id;
        }

        [[nodiscard]] uint32_t taskSize() const noexcept {
            return task_id_.toInt() < tasks_with_extra_item_ ? ept_ + 1 : ept_;;
        }
    private:
        WorldFilterResult* filter_result_ = nullptr;
        uint32_t ept_ = 0ull;
        uint32_t tasks_with_extra_item_ = 0ull;
        ParallelTaskId task_id_;
        TaskArchetypeIndex archetype_index_ = TaskArchetypeIndex::make(0u);
        uint32_t updated_in_archetype = 0ull;
    };

    class TaskGroup { // allows iterate over tasks
    public:
        TaskIterator begin() const noexcept {
            return TaskIterator {filter_result_, ParallelTaskId::make(0ull), task_count_};
        }

        ParallelTaskId end() const noexcept {
            return ParallelTaskId::make(task_count_);
        }

        uint32_t taskCount() const noexcept {
            return task_count_;
        }

        static TaskGroup make(WorldFilterResult& filter_result, uint32_t task_count) {
            TaskGroup result;
            result.filter_result_ = &filter_result;
            result.task_count_ = task_count;
            return result;
        }
    private:
        WorldFilterResult* filter_result_ = nullptr;
        uint32_t task_count_ = 0ull;
    };
#endif

}
