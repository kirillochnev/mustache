#pragma once

#include <mustache/ecs/world.hpp>
#include <mustache/ecs/base_job.hpp>
#include <mustache/ecs/task_view.hpp>
#include <mustache/ecs/world_filter.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>

#include <cstdint>

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
                                                   _ARGS MUSTACHE_RESTRICT_PTR ... pointers) noexcept(Info::is_noexcept) {
            using TargetType = typename std::conditional<Info ::is_const_this, const T, T>::type;
            TargetType& self = *static_cast<TargetType*>(this);
            if constexpr (Info::has_for_each_array) {
                invokeMethod(self, &T::forEachArray, world, count, invocation_index, pointers...);
            } else {
                const auto size = count.toInt();
                for(uint32_t i = 0u; i < size; ++i) {
                    invoke(self, world, invocation_index, pointers[i]...);
                    if constexpr(Info::FunctionInfo::Position::job_invocation >= 0) {
                        ++invocation_index.entity_index_in_task;
                        ++invocation_index.entity_index;
                    }
                }
            }
        }

        template <typename _C>
        static constexpr SharedComponent<_C> makeShared(_C* ptr) noexcept {
            static_assert(isComponentShared<_C>(), "Component is not shared");
            return SharedComponent<_C>{ptr};
        }

        template<size_t _I, typename _ViewType>
        MUSTACHE_INLINE static auto getComponentHandler(const _ViewType& view, ComponentIndex index) noexcept {
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
        MUSTACHE_INLINE void singleTask(World& world, ArchetypeGroup archetype_group, JobInvocationIndex invocation_index,
                                        const std::index_sequence<_I...>&, const std::index_sequence<_SI...>&) {
            auto shared_components = std::make_tuple(
                    getNullptr<_SI>()...
            );
            for (const auto& info : archetype_group) {
                auto& archetype = *info.archetype();
                archetype.getSharedComponents(shared_components);
                static const std::array<ComponentId, sizeof...(_I)> ids {
                        ComponentFactory::registerComponent<typename ComponentType<typename Info::FunctionInfo::
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
                                              getComponentHandler<_I>(array, component_indexes[_I])...,
                                              makeShared(std::get<_SI>(shared_components))...);
                    } else {
                        forEachArrayGenerated(world, array.arraySize(), invocation_index,
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
