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
            filter_result_.apply(world);
            return filter_result_.total_entity_count;
        }

        PerEntityJob() {
            filter_result_.mask_ = Info::componentMask();
        }
        MUSTACHE_INLINE void runCurrentThread(World& world) override {
            filter_result_.apply(world);
            static constexpr auto index_sequence = std::make_index_sequence<Info::FunctionInfo::components_count>();
            singleTask(PerEntityJobTaskId::make(0), TaskArchetypeIndex::make(0),
                       ArchetypeEntityIndex::make(0), filter_result_.total_entity_count, index_sequence);
        }

        MUSTACHE_INLINE void runParallel(World&, Dispatcher& dispatcher, uint32_t task_count) override {
            static constexpr auto index_sequence = std::make_index_sequence<Info::FunctionInfo::components_count>();

            for (const auto& task : splitByTasks(filter_result_, task_count)) {
                dispatcher.addParallelTask([task, this] {
                    singleTask(task.id, task.first_archetype, task.first_entity, task.size, index_sequence);
                });
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
        MUSTACHE_INLINE void applyPerArrayFunction(Archetype& archetype, ArchetypeEntityIndex first,
                ComponentArraySize count, JobInvocationIndex invocation_index) {
            if (count.toInt() < 1) {
                return;
            }

            static const std::array<ComponentId, sizeof...(_I)> ids {
                ComponentFactory::registerComponent<typename ComponentType<typename Info::FunctionInfo::
                template Component<_I>::type>::type>()...
            };
            std::array<ComponentIndex, sizeof...(_I)> component_indexes {
                archetype.getComponentIndex(ids[_I])...
            };

            auto view = archetype.getElementView(first);
            auto elements_rest = count.toInt();
            while (elements_rest != 0) {
                const auto arr_len = std::min(view.elementArraySize(), elements_rest);
                if constexpr (Info::FunctionInfo::Position::entity >= 0) {
                    forEachArrayGenerated(ComponentArraySize::make(arr_len), invocation_index,
                            view.getEntity<FunctionSafety::kUnsafe>(),
                            getComponentHandler<_I>(view, component_indexes[_I])...);
                } else {
                    forEachArrayGenerated(ComponentArraySize::make(arr_len), invocation_index,
                            getComponentHandler<_I>(view, component_indexes[_I])...);
                }

                view += arr_len;
                elements_rest -= arr_len;
            }
        }

        /// NOTE: world filtering required
        template<size_t... _I>
        MUSTACHE_INLINE void singleTask(PerEntityJobTaskId task_id, TaskArchetypeIndex archetype_index,
                                        ArchetypeEntityIndex begin, uint32_t count, const std::index_sequence<_I...>&) {
            JobInvocationIndex invocation_index;
            invocation_index.task_index = task_id;
            invocation_index.entity_index_in_task = PerEntityJobEntityIndexInTask::make(0);
            while (count != 0) {
                auto& archetype_info = filter_result_.filtered_archetypes[archetype_index.toInt()];
                const uint32_t num_free_entities_in_arch = archetype_info.entities_count - begin.toInt();
                const uint32_t objects_to_iterate = std::min(count, num_free_entities_in_arch);
                const auto array_size = ComponentArraySize::make(objects_to_iterate);
                applyPerArrayFunction<_I...>(*archetype_info.archetype, begin, array_size, invocation_index);
                count -= objects_to_iterate;
                begin = ArchetypeEntityIndex::make(0);
                ++archetype_index;
            }
        }

        WorldFilterResult filter_result_;
    };

}