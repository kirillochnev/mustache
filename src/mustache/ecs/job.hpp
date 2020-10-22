#pragma once

#include <cstdint>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache/ecs/world_filter.hpp>
#include <mustache//ecs/task_view.hpp>
#include <mustache/utils/dispatch.hpp>

namespace mustache {

    class Dispatcher;
    class Archetype;
    class World;

    enum class JobRunMode : uint32_t {
        kCurrentThread = 0u,
        kParallel = 1u,
        kSingleThread = 2u,
        kDefault = kCurrentThread,
    };

    class APerEntityJob {
    public:
        virtual ~APerEntityJob() = default;

        void run(World& world, Dispatcher& dispatcher, JobRunMode mode = JobRunMode::kDefault) {
            const auto entities_count = applyFilter(world);
            switch (mode) {
                case JobRunMode::kCurrentThread:
                    runCurrentThread(world);
                    break;
                case JobRunMode::kParallel:
                    runParallel(world, dispatcher, taskCount(dispatcher, entities_count));
                    break;
                case JobRunMode::kSingleThread:
                    runParallel(world, dispatcher, 1);
                    break;
            };
        }

        virtual void runParallel(World&, Dispatcher&, uint32_t num_tasks) = 0;
        virtual void runCurrentThread(World&) = 0;

        virtual uint32_t applyFilter(World&) noexcept = 0;
        virtual uint32_t taskCount(const Dispatcher& dispatcher, uint32_t entity_count) const noexcept {
            return std::min(entity_count, dispatcher.threadCount() + 1);
        }

    };

    template<typename T/*, typename _WorldFilter = DefaultWorldFilterResult*/>
    class PerEntityJob : public APerEntityJob {
    public:
        using Info = JobInfo<T>;
        using WorldFilterResult = DefaultWorldFilterResult;//_WorldFilter;

        virtual uint32_t applyFilter(World& world) noexcept override {
            static const ComponentIdMask empty_mask; // TODO: make correct mask
            const auto prev_version = WorldVersion::make(0); // TODO: replace null
            filter_result_.apply(world, /*empty_mask*/ filter_result_.mask_, prev_version);
            return filter_result_.total_entity_count;
        }

        PerEntityJob() {
            filter_result_.mask_ = Info::componentMask();
        }

        MUSTACHE_INLINE void runCurrentThread(World&) override {
            static constexpr auto index_sequence = std::make_index_sequence<Info::FunctionInfo::components_count>();
            PerEntityJobTaskId task_id = PerEntityJobTaskId::make(0);
            for (auto task : JobView::make(filter_result_, 1)) {
                singleTask(task, task_id, index_sequence);
                ++task_id;
            }
        }

        MUSTACHE_INLINE void runParallel(World&, Dispatcher& dispatcher, uint32_t task_count) override {
            static constexpr auto index_sequence = std::make_index_sequence<Info::FunctionInfo::components_count>();
            PerEntityJobTaskId task_id = PerEntityJobTaskId::make(0);
            for (auto task : JobView::make(filter_result_, task_count)) {
                dispatcher.addParallelTask([task, this, task_id] {
                    singleTask(task, task_id, index_sequence);
                });
                ++task_id;
            }
            dispatcher.waitForParallelFinish();
        }

    protected:

        template<typename... _ARGS>
        MUSTACHE_INLINE void forEachArrayGenerated(ComponentArraySize count, JobInvocationIndex& invocation_index,
                                   _ARGS MUSTACHE_RESTRICT_PTR ... pointers) {
            T& self = *static_cast<T*>(this);
            if constexpr (Info::has_for_each_array) {
                invokeMethod(self, &T::forEachArray, count, invocation_index, pointers...);
            } else {
                for(ComponentArraySize i = ComponentArraySize::make(0); i < count; ++i) {
                    invoke(self, invocation_index, pointers++...);
                    ++invocation_index.entity_index_in_task;
                }
            }
        }

        template<size_t _I, typename _ViewType>
        MUSTACHE_INLINE auto getComponentHandler(const _ViewType& view, ComponentIndex index) noexcept {
            using RequiredType = typename Info::FunctionInfo::template Component<_I>::type;
            using Component = typename ComponentType<RequiredType>::type;
            if constexpr (IsComponentRequired<RequiredType>::value) {
                auto ptr = view.template getData<FunctionSafety::kUnsafe>(index);
                return RequiredComponent<Component> {reinterpret_cast<Component*>(ptr)};
            } else {
                // TODO: it is possible to avoid per array check.
                auto ptr = view.template getData<FunctionSafety::kSafe>(index);
                return OptionalComponent<Component> {reinterpret_cast<Component*>(ptr)};
            }
        }

        template<size_t... _I>
        MUSTACHE_INLINE void singleTask(TaskView task_view, PerEntityJobTaskId task_id,
                const std::index_sequence<_I...>&) {
            JobInvocationIndex invocation_index;
            invocation_index.task_index = task_id;
            invocation_index.entity_index_in_task = PerEntityJobEntityIndexInTask::make(0);
            for (const auto& info : task_view) {
                auto& archetype = *info.archetype();
                static const std::array<ComponentId, sizeof...(_I)> ids {
                        ComponentFactory::registerComponent<typename ComponentType<typename Info::FunctionInfo::
                        template Component<_I>::type>::type>()...
                };
                std::array<ComponentIndex, sizeof...(_I)> component_indexes {
                        archetype.getComponentIndex(ids[_I])...
                };
                for (auto& array : ArchetypeView{filter_result_, info.archetype_index,
                                                      info.first_entity, info.current_size}) {
                    if constexpr (Info::FunctionInfo::Position::entity >= 0) {
                        forEachArrayGenerated(ComponentArraySize::make(array.arraySize()), invocation_index,
                                              array.getEntity<FunctionSafety::kUnsafe>(),
                                              getComponentHandler<_I>(array, component_indexes[_I])...);
                    } else {
                        forEachArrayGenerated(ComponentArraySize::make(array.arraySize()), invocation_index,
                                              getComponentHandler<_I>(array, component_indexes[_I])...);
                    }
                }
            }
        }

        WorldFilterResult filter_result_;
    };
}
