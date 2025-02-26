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
                                        JobInvocationIndex invocation_index) override;

    public:
        ComponentIdMask checkMask() const noexcept override;

        ComponentIdMask updateMask() const noexcept override;

        void onTaskBegin(World& world, TaskSize size, ParallelTaskId task_id) noexcept override;

        void onTaskEnd(World& world, TaskSize size, ParallelTaskId task_id) noexcept override;

        void onJobBegin(World& world, TasksCount count, JobSize total_entity_count, JobRunMode mode) noexcept override;

        void onJobEnd(World& world, TasksCount count, JobSize total_entity_count, JobRunMode mode) noexcept override;

        const char* nameCStr() const noexcept override;

        uint32_t applyFilter(World& world) noexcept override;

        ComponentIdMask version_check_mask;

        using Callback = std::function<void (ForEachArrayArgs)>;
        using TaskEvent = std::function<void (World&, TaskSize, ParallelTaskId)>;
        using JobEvent = std::function<void (World&, TasksCount, JobSize, JobRunMode)>;

        Callback callback;
        TaskEvent task_begin;
        TaskEvent task_end;

        JobEvent job_begin;
        JobEvent job_end;

        mustache::vector<ComponentRequest> component_requests;
        mustache::vector<SharedComponentId> shared_component_ids;
        std::string job_name = "NonTemplateJob";
        bool require_entity = false;
    };
}
