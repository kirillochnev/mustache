#pragma once

#include <mustache/ecs/world_filter.hpp>
#include <mustache/ecs/archetype.hpp>

namespace mustache {

    struct TaskInfo {
        uint32_t size;
        PerEntityJobTaskId id = PerEntityJobTaskId::make(0u);
        TaskArchetypeIndex first_archetype = TaskArchetypeIndex::make(0u);
        ArchetypeEntityIndex first_entity = ArchetypeEntityIndex::make(0u);
    };


    struct FilterResultArrayView : private ElementView {
        using FilrerResult = DefaultWorldFilterResult::ArchetypeFilterResult;

        FilterResultArrayView(DefaultWorldFilterResult& filter_result, TaskArchetypeIndex archetype_index,
                ArchetypeEntityIndex first_entity, uint32_t size):
                ElementView{filter_result.filtered_archetypes[archetype_index.toInt()].archetype->getElementView(first_entity)},
                dist_to_end_ {std::min(size, archetype_->size() - first_entity.toInt())},
                filter_result_ {&filter_result.filtered_archetypes[archetype_index.toInt()]} {
            if (dist_to_end_ > 0) {
                const auto index_in_archetype = globalIndex().toInt();

                auto dist_to_block_end = filter_result_->blocks[current_block_].end - index_in_archetype;
                if (dist_to_block_end == 0u) {
                    ++current_block_;
                    dist_to_block_end = filter_result_->blocks[current_block_].end - index_in_archetype;
                }
                array_size_ = std::min(dist_to_block_end, std::min(elementArraySize(), dist_to_end_));
            }
        }

        FilterResultArrayView& begin() { return *this; }
        nullptr_t end() const noexcept { return nullptr; }
        bool operator != (nullptr_t) const noexcept { return dist_to_end_ != 0u; }
        const FilterResultArrayView& operator*() const noexcept { return *this; }

        FilterResultArrayView& operator++() noexcept {
            dist_to_end_ -= array_size_;
            *this += array_size_;
            if (dist_to_end_ > 0) {
                const auto index_in_archetype = globalIndex().toInt();

                auto dist_to_block_end = filter_result_->blocks[current_block_].end - index_in_archetype;
                if (dist_to_block_end == 0u) {
                    ++current_block_;
                    dist_to_block_end = filter_result_->blocks[current_block_].end - index_in_archetype;
                }
                array_size_ = std::min(dist_to_block_end, std::min(elementArraySize(), dist_to_end_));
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
        uint32_t current_block_ = 0u;
        uint32_t array_size_ = 0u;
        uint32_t dist_to_end_ = 0u;
        FilrerResult* filter_result_ = nullptr;
    };

    struct ArchetypeIterator {
        ArchetypeIterator(const TaskInfo& info, DefaultWorldFilterResult& fr):
                dist_to_end{info.size},
                archetype_index{info.first_archetype},
                filtered_archetypes{&fr.filtered_archetypes},
                first_entity{info.first_entity} {
            const auto& archetype_info = (*filtered_archetypes)[archetype_index.toInt()];
            const uint32_t num_free_entities_in_arch = archetype_info.entities_count - info.first_entity.toInt();
            current_size = std::min(dist_to_end, num_free_entities_in_arch);
        }
        void incrementArchetype() {
            dist_to_end -= current_size;
            if (dist_to_end > 0) {
                ++archetype_index;
                const auto &archetype_info = (*filtered_archetypes)[archetype_index.toInt()];
                const uint32_t num_free_entities_in_arch = archetype_info.entities_count;
                current_size = std::min(dist_to_end, num_free_entities_in_arch);
                first_entity = ArchetypeEntityIndex::make(0);
            } else {
                current_size = 0;
            }
        }

        ArchetypeIterator& operator++() noexcept {
            incrementArchetype();
            return *this;
        }

        Archetype* archetype() const noexcept {
            return (*filtered_archetypes)[archetype_index.toInt()].archetype;
        }
        const ArchetypeIterator& operator*() const noexcept { return *this; }
        bool operator != (nullptr_t) const noexcept {  return dist_to_end != 0u; }
        ArchetypeIterator begin() const noexcept { return *this; }
        nullptr_t end() const noexcept { return nullptr; }

        uint32_t current_size {0u};
        uint32_t dist_to_end {0u};
        TaskArchetypeIndex archetype_index = TaskArchetypeIndex::make(0u);
        decltype(DefaultWorldFilterResult::filtered_archetypes)* filtered_archetypes = nullptr;
        ArchetypeEntityIndex first_entity = ArchetypeEntityIndex::make(0u);
    };

    struct TaskIterator {
    public:
        struct End {
            PerEntityJobTaskId task_id_;
        };

        bool operator!=(const End& end) const noexcept {
            return task_info_.id.isValid() && task_info_.id != end.task_id_;
        }

        void updateTaskSize() noexcept {
            task_info_.size = task_info_.id.toInt() < tasks_with_extra_item_ ? ept_ + 1 : ept_;
        }

        TaskIterator& operator++() noexcept {
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

        ArchetypeIterator operator*() const noexcept {
            return ArchetypeIterator{task_info_, *filter_result_};
        }

        TaskIterator(DefaultWorldFilterResult& filter_result, uint32_t num_tasks):
                filter_result_{&filter_result},
                ept_{filter_result.total_entity_count / num_tasks},
                tasks_with_extra_item_ {filter_result.total_entity_count - num_tasks * ept_} {
            updateTaskSize();
        }

    private:
        DefaultWorldFilterResult* filter_result_;
        uint32_t ept_;
        uint32_t tasks_with_extra_item_;
        TaskInfo task_info_;
    };

    MUSTACHE_INLINE auto splitByTasks(DefaultWorldFilterResult& filter_result, uint32_t num_tasks) {
        struct Result {
            Result(DefaultWorldFilterResult &filter_result, uint32_t num_tasks) :
                    begin_{filter_result, num_tasks},
                    end_{PerEntityJobTaskId::make(num_tasks)} {

            }

            TaskIterator begin() noexcept {
                return begin_;
            }

            TaskIterator::End end() const noexcept {
                return end_;
            }

        private:
            TaskIterator begin_;
            TaskIterator::End end_;
        };

        return Result{filter_result, num_tasks};
    }
}
