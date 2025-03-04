#include "non_template_job.hpp"

using namespace mustache;

void NonTemplateJob::singleTask(World&, ArchetypeGroup archetype_group, JobInvocationIndex invocation_index) {
    mustache::vector<const void*> shared_components(shared_component_ids.size());
    mustache::vector<ComponentIndex> component_indexes(component_requests.size());
    mustache::vector<void*> component_ptr(component_requests.size());
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

ComponentIdMask NonTemplateJob::checkMask() const noexcept {
    return version_check_mask;
}

ComponentIdMask NonTemplateJob::updateMask() const noexcept {
    ComponentIdMask write_mask;
    for (const auto& request : component_requests) {
        write_mask.set(request.id, !request.is_const);
    }
    return write_mask;
}

void NonTemplateJob::onTaskBegin(World& world, TaskSize size, ParallelTaskId task_id) noexcept {
    if (task_begin) {
        task_begin(world, size, task_id);
    }
}

void NonTemplateJob::onTaskEnd(World& world, TaskSize size, ParallelTaskId task_id) noexcept {
    if (task_end) {
        task_end(world, size, task_id);
    }
}

void NonTemplateJob::onJobBegin(World& world, TasksCount count, JobSize total_entity_count, JobRunMode mode) noexcept {
    if (job_begin) {
        job_begin(world, count, total_entity_count, mode);
    }
}

void NonTemplateJob::onJobEnd(World& world, TasksCount count, JobSize total_entity_count, JobRunMode mode) noexcept {
    if (job_end) {
        job_end(world, count, total_entity_count, mode);
    }
}

const char* NonTemplateJob::nameCStr() const noexcept {
    return job_name.c_str();
}

uint32_t NonTemplateJob::applyFilter(World& world) noexcept {
    filter_result_.mask = ComponentIdMask::null();
    filter_result_.shared_component_mask = SharedComponentIdMask::null();

    for (const auto& request : component_requests) {
        filter_result_.mask.set(request.id, request.is_required);
    }

    for (const auto& id : shared_component_ids) {
        filter_result_.shared_component_mask.set(id, true);
    }

    return BaseJob::applyFilter(world);
}
