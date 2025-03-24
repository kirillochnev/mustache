#pragma once

#include <mustache/ecs/world.hpp>
#include <mustache/ecs/base_job.hpp>
#include <mustache/ecs/task_view.hpp>
#include <mustache/ecs/world_filter.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache/utils/profiler.hpp>

#include <cstdint>

namespace mustache {

    class Dispatcher;
    class Archetype;
    class World;

    template<typename _Job>
    struct JobHelper {
        using Info = JobInfo<_Job>;

        template<size_t>
        MUSTACHE_INLINE static void incPtrs() {
        }
        template<size_t _Count, typename _Ptr, typename... _Other>
        MUSTACHE_INLINE static void incPtrs(_Ptr& ptr, _Other&... other) {
            ptr += _Count;
            incPtrs<_Count>(other...);
        }
        MUSTACHE_INLINE static void incInvocationIndex(JobInvocationIndex& invocation_index) noexcept {
            if constexpr(Info::FunctionInfo::Position::job_invocation >= 0) {
                ++invocation_index.entity_index_in_task;
                ++invocation_index.entity_index;
            }
        }

        template<size_t _I, typename _View>
        static auto getComponentHandler(_View&& view, ComponentIndex index) noexcept {
            constexpr auto Safety = FunctionSafety::kUnsafe;

            using ArgType = typename Info::FunctionInfo::template UniqueComponentType<_I>::type;
            using Component = typename ComponentType<ArgType>::type;
            if constexpr (IsComponentRequired<ArgType>::value) {
                auto ptr = view.template getData<Safety>(index);
                return RequiredComponent<Component> {reinterpret_cast<Component*>(ptr)};
            } else {
                void* ptr = nullptr;
                if (!index.isNull()) {
                    ptr = view.template getData<Safety>(index);
                }
                return OptionalComponent<Component> {reinterpret_cast<Component*>(ptr)};
            }
        }
        template <typename _C>
        static constexpr SharedComponent<_C> makeShared(_C* ptr) noexcept {
            static_assert(isComponentShared<_C>(), "Component is not shared");
            return SharedComponent<_C>{ptr};
        }

        template<size_t _I>
        static auto getComponentIndex(const Archetype& archetype, ComponentId id) noexcept {
            using ArgType = typename Info::FunctionInfo::template UniqueComponentType<_I>::type;
            constexpr bool is_required = IsComponentRequired<ArgType>::value;
            constexpr auto Safety = is_required ? FunctionSafety::kUnsafe : FunctionSafety::kSafe;
            return archetype.getComponentIndex<Safety>(id);
        }

        template<size_t _I>
        constexpr static bool isComponentMutable() {
            using FunctionInfo = typename Info::FunctionInfo;
            using Component = typename ComponentType<
                    typename FunctionInfo::template UniqueComponentType<_I>::type>::type;
            return IsComponentMutable<Component>::value;
        }

        template<size_t... _I>
        static void updateVersion(WorldVersion version, Archetype& archetype,
                                  const std::array<ComponentIndex, sizeof...(_I)>& component_indexes) noexcept {
            static constexpr std::array<bool, sizeof...(_I)> is_mutable = {
                    isComponentMutable<_I>()...
            };
            const auto last_chunk = ChunkIndex::make(archetype.chunkCount());
            for (auto chunk = ChunkIndex::make(0); chunk != last_chunk; ++chunk) {
                for (uint32_t component = 0; component < sizeof...(_I); ++component) {
                    if (is_mutable[component]) {
                        archetype.setVersion(version, chunk, component_indexes[component]);
                    }
                }
            }
        }
        template<size_t _ComponentIndex>
        static constexpr auto getNullptr() noexcept {
            using Type = typename Info::FunctionInfo::template SharedComponentType<_ComponentIndex>::type;
            using ResultType = typename ComponentType<Type>::type;
            return static_cast<const ResultType*>(nullptr);
        }
        template<JobUnroll _Unroll, typename... _ARGS>
        static constexpr bool needUnroll() noexcept {
            if constexpr (_Unroll == JobUnroll::kEnabled) {
                return true;
            }
            size_t total_size = 0u;
            const std::initializer_list<size_t > sizes {sizeof(_ARGS)...};
            for (auto size : sizes) {
                total_size += size;
            }
            return total_size > 64;
        }

        template<JobUnroll _Unroll, typename _F, typename... _ARGS>
        static void forEachInArrays(World& world, _F&& function,
                                    JobInvocationIndex& invocation_index, size_t count, _ARGS&&... args) {
            if constexpr (!needUnroll<_Unroll, _ARGS...>()) {
                for (size_t i = 0u; i < count; ++i) {
                    invoke(function, world, invocation_index, args[i]...);
                    incInvocationIndex(invocation_index);
                }
            } else {
                const size_t size = count / 4u;
                // TODO: find the way to make compiler unroll this loop
                for (size_t i = 0u; i < size; ++i) {
                    invoke(function, world, invocation_index, args[0]...);
                    incInvocationIndex(invocation_index);

                    invoke(function, world, invocation_index, args[1]...);
                    incInvocationIndex(invocation_index);

                    invoke(function, world, invocation_index, args[2]...);
                    incInvocationIndex(invocation_index);

                    invoke(function, world, invocation_index, args[3]...);
                    incInvocationIndex(invocation_index);

                    incPtrs<4u>(args...);
                }

                using TailFunction = void (*)(const _F&, World&, JobInvocationIndex&, _ARGS&...);
                static constexpr TailFunction tails[4] {
                        [](const _F&, World&, JobInvocationIndex&, _ARGS&...) {},
                        [](const _F& f, World& _world, JobInvocationIndex& ii, _ARGS&... _pointers) {
                            invoke(f, _world, ii, _pointers[0]...);
                            incInvocationIndex(ii);
                        },
                        [](const _F& f, World& _world, JobInvocationIndex& ii, _ARGS&... _pointers) {
                            invoke(f, _world, ii, _pointers[0]...);
                            incInvocationIndex(ii);

                            invoke(f, _world, ii, _pointers[1]...);
                            incInvocationIndex(ii);
                        },
                        [](const _F& f, World& _world, JobInvocationIndex& ii, _ARGS&... _pointers) {
                            invoke(f, _world, ii, _pointers[0]...);
                            incInvocationIndex(ii);

                            invoke(f, _world, ii, _pointers[1]...);
                            incInvocationIndex(ii);

                            invoke(f, _world, ii, _pointers[2]...);
                            incInvocationIndex(ii);
                        }};
                tails[count % 4](function, world, invocation_index, args...);
            }
        }
    };

    template<typename T, JobUnroll _Unroll = JobUnroll::kAuto>
    class PerEntityJob : public BaseJob {
    public:
        using Info = JobInfo<T>;

        PerEntityJob() {
            filter_result_.mask = Info::componentMask();
            filter_result_.shared_component_mask = Info::sharedComponentMask();
        }

        ComponentIdMask checkMask() const noexcept override {
            return ComponentIdMask::null();
        }

        ComponentIdMask updateMask() const noexcept override {
            return Info::updateMask();
        }

        virtual const char* nameCStr() const noexcept override {
            static const auto job_type_name = type_name<T>();
            static const auto result = job_type_name.c_str();
            return result;
        }

        void singleTask(World& world, ArchetypeGroup task, JobInvocationIndex invocation_index) override {
            static constexpr auto unique_components = std::make_index_sequence<Info::FunctionInfo::components_count>();
            static constexpr auto shared_components = std::make_index_sequence<Info::FunctionInfo::shared_components_count>();
            singleTask(world, task, invocation_index, unique_components, shared_components);
        }

    protected:
        template<typename... _ARGS>
        MUSTACHE_INLINE void forEachArrayGenerated(World& world, ComponentArraySize count,
                                                   JobInvocationIndex& invocation_index,
                                                   _ARGS... pointers) noexcept(Info::is_noexcept) {
            using TargetType = typename std::conditional<Info ::is_const_this, const T, T>::type;
            TargetType& self = *static_cast<TargetType*>(this);
            if constexpr (Info::has_for_each_array) {
                invokeMethod(self, &T::forEachArray, world, count, invocation_index, pointers...);
            } else {
                JobHelper<TargetType>::template forEachInArrays<_Unroll>(world, self, invocation_index, count.toInt(), pointers...);
            }
        }
        template<size_t... _I, size_t... _SI>
        MUSTACHE_INLINE void singleTask(World& world, ArchetypeGroup archetype_group, JobInvocationIndex invocation_index,
                                        const std::index_sequence<_I...>&, const std::index_sequence<_SI...>&) {
            auto shared_components = std::make_tuple(
                    JobHelper<T>::template getNullptr<_SI>()...
            );
            for (const auto& info : archetype_group) {
                auto& archetype = *info.archetype();
                archetype.getSharedComponents(shared_components);
                static const std::array<ComponentId, sizeof...(_I)> ids {
                        ComponentFactory::instance().registerComponent<typename ComponentType<typename Info::FunctionInfo::
                        template UniqueComponentType<_I>::type>::type>()...
                };
                std::array<ComponentIndex, sizeof...(_I)> component_indexes {
                        archetype.getComponentIndex(ids[_I])...
                };

                for (auto array : ArrayView::make(filter_result_, info.archetype_index,
                                                  info.first_entity, info.current_size)) {

                    if constexpr (Info::FunctionInfo::Position::entity >= 0) {
                        forEachArrayGenerated(world, array.arraySize(), invocation_index,
                                              RequiredComponent<Entity>(array.template getEntity<FunctionSafety::kUnsafe>()),
                                              JobHelper<T>:: template getComponentHandler<_I>(array, component_indexes[_I])...,
                                              JobHelper<T>::makeShared(std::get<_SI>(shared_components))...);
                    } else {
                        forEachArrayGenerated(world, array.arraySize(), invocation_index,
                                              JobHelper<T>:: template getComponentHandler<_I>(array, component_indexes[_I])...,
                                              JobHelper<T>::makeShared(std::get<_SI>(shared_components))...);
                    }
                }
            }
        }

    };
    // task with no filter stage

    template<typename _Function, JobUnroll _Unroll = JobUnroll::kAuto>
    class SimpleTask {
    private:
        using Info = JobInfo<_Function>;
        ComponentFactory* factory;
        ComponentIdMask component_id_mask;

        template<typename _F, size_t... _I, size_t... _SI>
        void runWithIndexSequence(World& world, _F&& function,
                                  std::index_sequence<_I...>&&, std::index_sequence<_SI...>&&) {
            auto& entities = world.entities();
            const auto archetypes_count = entities.getArchetypesCount();
            if (archetypes_count < 1) {
                return;
            }
            using FunctionInfo = typename Info::FunctionInfo;
            constexpr auto Safety = FunctionSafety::kUnsafe;
            static const std::array<ComponentId, sizeof...(_I) > unique_ids {
                    factory->registerComponent<typename ComponentType<
                            typename FunctionInfo::template UniqueComponentType<_I>::type>::type>()...
            };
            static const std::array<ComponentId, sizeof...(_SI) > shared_ids {
                    factory->registerComponent<typename ComponentType<
                            typename FunctionInfo::template SharedComponentType<_SI>::type>::type>()...
            };
            auto shared_components = std::make_tuple(
                    JobHelper<_Function>::template getNullptr<_SI>()...
            );
            const auto world_version = world.version();
            std::array<ComponentIndex, sizeof...(_I)> component_indexes;
            JobInvocationIndex invocation_index;
            invocation_index.entity_index_in_task = ParallelTaskItemIndexInTask::make(0);
            invocation_index.entity_index = ParallelTaskGlobalItemIndex::make(0);
            invocation_index.thread_id = ThreadId::make(0);
            bool was_locked = false;
            for (size_t ai = 0; ai < archetypes_count; ++ai) {
                const auto archetype_index = ArchetypeIndex::make(ai);
                auto& arch = entities.getArchetype<Safety>(archetype_index);
                auto entities_to_process = arch.size();
                if (entities_to_process  < 1 || !arch.isMatch(component_id_mask)) {
                    continue;
                }
                if (!was_locked) {
                    world.entities().lock();
                    was_locked = true;
                }
                arch.getSharedComponents(shared_components);
                component_indexes = {
                        JobHelper<_Function>::template getComponentIndex<_I>(arch, unique_ids[_I])...
                };

                JobHelper<_Function>::template updateVersion<_I...>(world_version, arch, component_indexes);

                auto view = arch.getElementView(ArchetypeEntityIndex::make(0));
                while (entities_to_process > 0) {
                    const auto chunk_size = view.distToChunkEnd();
                    JobHelper<_Function>::template forEachInArrays<_Unroll>(world, function, invocation_index, chunk_size, view.getEntity(),
                                                                            JobHelper<_Function>::template getComponentHandler<_I>(view, component_indexes[_I])...,
                                                                            JobHelper<_Function>::makeShared(std::get<_SI>(shared_components))...
                    );

                    entities_to_process -= chunk_size;
                    view += chunk_size;
                }
            }
            if (was_locked) {
                world.entities().unlock();
            }
        }
    public:
        SimpleTask(ComponentFactory* _factory, const ComponentIdMask& _mask):
                factory {_factory},
                component_id_mask {_mask} {
        }
        SimpleTask():
                SimpleTask(&ComponentFactory::instance(), Info::componentMask()) {
        }

        template<typename _F>
        void run(World& world, _F&& function) {
#if MUSTACHE_PROFILER_LVL >= 3
            const auto profiler_msg = mustache::type_name<_Derived>() + "::run()";
            MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
            constexpr auto unique = Info::FunctionInfo::componentsCount();
            constexpr auto shared = Info::FunctionInfo::sharedComponentsCount();
            runWithIndexSequence(world, std::forward<_F>(function),
                                 std::make_index_sequence<unique>(), std::make_index_sequence<shared>());
        }
        void run(World& world) {
            if constexpr (std::is_base_of_v<SimpleTask<_Function>, _Function>) {
                run(world, *static_cast<_Function*>(this));
            } else {
                static_assert(std::is_base_of_v<SimpleTask<_Function>, _Function>);
            }
        }
    };

    template<JobUnroll _Unroll, typename _F, typename... ARGS>
    void EntityManager::forEachWithArgsTypes(_F&& function, JobRunMode mode) {
        if (mode == JobRunMode::kCurrentThread) {
            static SimpleTask<_F> job;
            job.run(world_, std::forward<_F>(function));

        } else {
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
}
