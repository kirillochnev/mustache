#pragma once

#include <mustache/ecs/world_filter.hpp>

namespace mustache {

    struct TaskArrayView {
        struct ArrayHandler {
            ArrayHandler& operator++() noexcept {
                auto& archetype_info = filter_result_->filtered_archetypes[archetype_index.toInt()];
                const uint32_t num_free_entities_in_arch = archetype_info.entities_count - entity_index.toInt();
                current_iteration_array_size = std::min(entities_count, num_free_entities_in_arch);

//                applyPerArrayFunction<_I...>(*archetype_info.archetype, begin, array_size, invocation_index);

                entities_count -= current_iteration_array_size;
                entity_index = ArchetypeEntityIndex::make(0);
                ++archetype_index;
                return *this;
            }
            ArrayHandler& operator*() noexcept {
                return *this;
            }

            bool operator!=(nullptr_t) const noexcept {
                return entities_count != 0;
            }
            DefaultWorldFilterResult* filter_result_ = nullptr;
            uint32_t entities_count = 0u;
            uint32_t current_iteration_array_size = 0u;
            TaskArchetypeIndex archetype_index = TaskArchetypeIndex::make(0u);
            ArchetypeEntityIndex entity_index = ArchetypeEntityIndex::make(0u);
            JobInvocationIndex invocation_index;
        };

        ArrayHandler begin_;

        ArrayHandler begin() const noexcept {return ArrayHandler{};}
        nullptr_t end() const noexcept {return nullptr;}
    };

    struct TaskElementView {
        struct Element {
            Element& operator++() noexcept {
                return *this;
            }
            Element& operator*() noexcept {
                return *this;
            }
            bool operator!=(nullptr_t) const noexcept {
                return true;
            }
        };
        Element begin() const noexcept {return Element{};}
        nullptr_t end() const noexcept {return nullptr;}
    };

    struct TaskInfo {
        TaskArrayView arrayView() const noexcept {
            return TaskArrayView{};
        }
        TaskElementView elementView() const noexcept {
            return TaskElementView{};
        }

        uint32_t current_task_size;
        PerEntityJobTaskId task_id = PerEntityJobTaskId::make(0u);
        TaskArchetypeIndex first_archetype = TaskArchetypeIndex::make(0u);
        ArchetypeEntityIndex first_entity = ArchetypeEntityIndex::make(0u);
    };

    struct TaskViewIterator {
    public:
        struct End {
            PerEntityJobTaskId task_id_;
        };

        bool operator!=(const End& end) const noexcept {
            return task_info_.task_id.isValid() && task_info_.task_id != end.task_id_;
        }

        void updateTaskSize() noexcept {
            task_info_.current_task_size = task_info_.task_id.toInt() < tasks_with_extra_item_ ? ept_ + 1 : ept_;
        }

        TaskViewIterator& operator++() noexcept {
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
        friend struct TaskView;
        TaskViewIterator(DefaultWorldFilterResult& filter_result, uint32_t num_tasks):
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

    struct TaskView {
        TaskView(DefaultWorldFilterResult& filter_result, uint32_t num_tasks):
                begin_{filter_result, num_tasks},
                end_ {PerEntityJobTaskId::make(num_tasks)} {

        }

        TaskViewIterator begin() noexcept {
            return begin_;
        }
        TaskViewIterator::End end() const noexcept {
            return end_;
        }
    private:
        TaskViewIterator begin_;
        TaskViewIterator::End end_;
    };
}
