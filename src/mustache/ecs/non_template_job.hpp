#include <mustache/ecs/base_job.hpp>

namespace mustache {
    struct MUSTACHE_EXPORT NonTemplateJob : public BaseJob {

        using ComponentPtr = void*;
        using SharedComponentPtr = const void*;

        struct ForEachArrayArgs {
            Entity* entities;
            ComponentPtr* components;
            SharedComponentPtr* shared_components;
            ComponentArraySize count;
            JobInvocationIndex invocation_index;
        };

        struct ComponentRequest {
            ComponentId id;
            bool is_const;
            bool is_required = true;
        };

        void singleTask(World&, ArchetypeGroup archetype_group,
                                        JobInvocationIndex invocation_index) override {
            std::vector<const void*> shared_components(shared_component_ids.size());
            std::vector<ComponentIndex> component_indexes(component_requests.size());
            std::vector<void*> component_ptr(component_requests.size());
            ForEachArrayArgs args;
            args.shared_components = shared_components.data();
            args.components = component_ptr.data();
            args.invocation_index = invocation_index;

            const auto update_per_archetype_data = [&](Archetype& archetype) {
                for (uint32_t i = 0; i < component_requests.size(); ++i) {
                    component_indexes[i] = archetype.getComponentIndex(component_requests[i].id);
                }

                for (uint32_t i = 0; i < shared_components.size(); ++i) {
                    const auto index = archetype.sharedComponentIndex(shared_component_ids[i]);
                    shared_components[i] = archetype.getSharedComponent(index);
                }
            };
            const auto update_per_array_data = [&](const ArrayView& array) {
                for (uint32_t i = 0; i < component_indexes.size(); ++i) {
                    if (component_requests[i].is_required) {
                        component_ptr[i] = array.getData<FunctionSafety::kUnsafe>(component_indexes[i]);
                    } else {
                        component_ptr[i] = array.getData<FunctionSafety::kSafe>(component_indexes[i]);
                    }
                }

                args.entities = require_entity ? array.getEntity() : nullptr;
            };

            for (const auto& info : archetype_group) {
                update_per_archetype_data(*info.archetype());

                for (auto array : ArrayView::make(filter_result_, info.archetype_index,
                                                  info.first_entity, info.current_size)) {
                    update_per_array_data(array);
                    args.count = array.arraySize();

                    callback(args);

                    args.invocation_index.entity_index = ParallelTaskGlobalItemIndex::make(
                            args.count.toInt() + args.invocation_index.entity_index.toInt());
                    args.invocation_index.entity_index_in_task = ParallelTaskItemIndexInTask::make(
                            args.count.toInt() + args.invocation_index.entity_index_in_task.toInt());
                }
            }
        }

    public:
        ComponentIdMask checkMask() const noexcept override {
            return version_check_mask;
        }

        ComponentIdMask updateMask() const noexcept override {
            ComponentIdMask write_mask;
            for (const auto& request : component_requests) {
                write_mask.set(request.id, !request.is_const);
            }
            return write_mask;
        }

        void onTaskBegin(World& world, TaskSize size, ParallelTaskId task_id) noexcept override {
            if (task_begin) {
                task_begin(world, size, task_id);
            }
        }

        void onTaskEnd(World& world, TaskSize size, ParallelTaskId task_id) noexcept override {
            if (task_end) {
                task_end(world, size, task_id);
            }
        }

        void onJobBegin(World& world, TasksCount count, JobSize total_entity_count, JobRunMode mode) noexcept override {
            if (job_begin) {
                job_begin(world, count, total_entity_count, mode);
            }
        }

        void onJobEnd(World& world, TasksCount count, JobSize total_entity_count, JobRunMode mode) noexcept override {
            if (job_end) {
                job_end(world, count, total_entity_count, mode);
            }
        }

        std::string name() const noexcept override {
            return job_name;
        }

        uint32_t applyFilter(World& world) noexcept override {
            filter_result_.mask = {};
            filter_result_.shared_component_mask = {};

            for (const auto& request : component_requests) {
                filter_result_.mask.set(request.id, request.is_required);
            }

            for (const auto& id : shared_component_ids) {
                filter_result_.shared_component_mask.set(id, true);
            }

            return BaseJob::applyFilter(world);
        }

        ComponentIdMask version_check_mask;

        using Callback = std::function<void (ForEachArrayArgs)>;
        using TaskEvent = std::function<void (World&, TaskSize, ParallelTaskId)>;
        using JobEvent = std::function<void (World&, TasksCount, JobSize, JobRunMode)>;

        Callback callback;
        TaskEvent task_begin;
        TaskEvent task_end;

        JobEvent job_begin;
        JobEvent job_end;

        std::vector<ComponentRequest> component_requests;
        std::vector<SharedComponentId> shared_component_ids;
        std::string job_name = "NonTemplateJob";
        bool require_entity = false;
    };
}
