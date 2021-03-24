#pragma once

#include <cstdint>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache/ecs/world_filter.hpp>
#include <mustache//ecs/task_view.hpp>
#include <mustache/ecs/base_job.hpp>
#include <mustache/utils/dispatch.hpp>

namespace mustache {

    class Dispatcher;
    class Archetype;
    class World;

    template<typename T>
    class PerEntityJob : public BaseJob {
    public:
        using Info = JobInfo<T>;

        PerEntityJob() {
            filter_result_.mask = Info::componentMask();
            filter_result_.shared_component_mask = Info::sharedComponentMask();
        }

        ComponentIdMask checkMask() const noexcept override {
            return ComponentIdMask{};
        }

        ComponentIdMask updateMask() const noexcept override {
            return Info::updateMask();
        }

        MUSTACHE_INLINE void runCurrentThread(World& world) override {
            static constexpr auto unique_components = std::make_index_sequence<Info::FunctionInfo::components_count>();
            static constexpr auto shared_components = std::make_index_sequence<Info::FunctionInfo::shared_components_count>();
            auto& dispatcher = world.dispatcher();

            JobInvocationIndex invocation_index;
            invocation_index.task_index = ParallelTaskId::make(0);
            invocation_index.thread_id = dispatcher.currentThreadId();
            invocation_index.entity_index_in_task = ParallelTaskItemIndexInTask::make(0);
            invocation_index.entity_index = ParallelTaskGlobalItemIndex::make(0);

            const auto thread_id = dispatcher.currentThreadId();
            for (auto task : TaskGroup::make(filter_result_, 1)) {
                singleTask(task, invocation_index, unique_components, shared_components);
                ++invocation_index.task_index;
            }
        }

        MUSTACHE_INLINE void runParallel(World& world, uint32_t task_count) override {
            static constexpr auto unique_components = std::make_index_sequence<Info::FunctionInfo::components_count>();
            static constexpr auto shared_components = std::make_index_sequence<Info::FunctionInfo::shared_components_count>();

            auto& dispatcher = world.dispatcher();
            JobInvocationIndex invocation_index;
            invocation_index.entity_index = ParallelTaskGlobalItemIndex::make(0);
            invocation_index.entity_index_in_task = ParallelTaskItemIndexInTask::make(0);
            invocation_index.task_index = ParallelTaskId::make(0);
            for (TaskView task : TaskGroup::make(filter_result_, task_count)) {
                dispatcher.addParallelTask([task, this, invocation_index](ThreadId thread_id) mutable {
                    invocation_index.thread_id = thread_id;
                    singleTask(task, invocation_index, unique_components, shared_components);
                });
                ++invocation_index.task_index;
                invocation_index.entity_index = ParallelTaskGlobalItemIndex::make(invocation_index.entity_index.toInt() + task.dist_to_end);
            }
            dispatcher.waitForParallelFinish();
        }

        virtual std::string name() const noexcept override {
            return type_name<T>();
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
                    ++invocation_index.entity_index;
                }
            }
        }

        template <typename _C>
        static constexpr SharedComponent<_C> makeShared(_C* ptr) noexcept {
            static_assert(isComponentShared<_C>(), "Component is not shared");
            return SharedComponent<_C>{ptr};
        }

        template<size_t _I, typename _ViewType>
        MUSTACHE_INLINE auto getComponentHandler(const _ViewType& view, ComponentIndex index) noexcept {
            using RequiredType = typename Info::FunctionInfo::template UniqueComponentType<_I>::type;
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

        template<size_t _ComponentIndex>
        static constexpr auto getNullptr() noexcept {
            using Type = typename Info::FunctionInfo::template SharedComponentType<_ComponentIndex>::type;
            using ResultType = typename ComponentType<Type>::type;
            return static_cast<const ResultType*>(nullptr);
        }

        template<size_t... _I, size_t... _SI>
        MUSTACHE_INLINE void singleTask(TaskView task_view, JobInvocationIndex invocation_index,
                                        const std::index_sequence<_I...>&, const std::index_sequence<_SI...>&) {
            auto shared_components = std::make_tuple(
                    getNullptr<_SI>()...
            );
            for (const auto& info : task_view) {
                auto& archetype = *info.archetype();
                archetype.getSharedComponents(shared_components);
                static const std::array<ComponentId, sizeof...(_I)> ids {
                        ComponentFactory::registerComponent<typename ComponentType<typename Info::FunctionInfo::
                        template UniqueComponentType<_I>::type>::type>()...
                };
                std::array<ComponentIndex, sizeof...(_I)> component_indexes {
                        archetype.getComponentIndex(ids[_I])...
                };
                for (auto& array : ArrayView{filter_result_, info.archetype_index,
                                             info.first_entity, info.current_size}) {
                    if constexpr (Info::FunctionInfo::Position::entity >= 0) {
                        forEachArrayGenerated(ComponentArraySize::make(array.arraySize()), invocation_index,
                                              RequiredComponent<Entity>(array.getEntity<FunctionSafety::kUnsafe>()),
                                              getComponentHandler<_I>(array, component_indexes[_I])...,
                                              makeShared(std::get<_SI>(shared_components))...);
                    } else {
                        forEachArrayGenerated(ComponentArraySize::make(array.arraySize()), invocation_index,
                                              getComponentHandler<_I>(array, component_indexes[_I])...,
                                              makeShared(std::get<_SI>(shared_components))...);
                    }
                }
            }
        }

    };

    template<typename _F, typename... ARGS>
    void EntityManager::forEachWithArgsTypes(_F&& function, JobRunMode mode) {
        static std::string job_name = "";
        if (job_name.empty()) {
            std::string str = ((type_name<ARGS>() + ", ") + ... + "");
            if (!str.empty()) {
                str.pop_back();
                str.pop_back();
            }
            job_name = "ForEachJob<" + str + ">";
        }
        struct TmpJob : public PerEntityJob<TmpJob> {
            TmpJob(_F&& f):
                    func{std::forward<_F>(f)} {
                this->disableBenchmark();
            }
            _F&& func;
            void operator() (ARGS... args) {
                func(std::forward<ARGS>(args)...);
            }

            virtual std::string name() const noexcept override {
                return job_name;
            }
        };
        TmpJob job = std::forward<_F>(function);
        job.run(world_, mode);
    }
}
