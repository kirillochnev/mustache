#pragma once

#include <cstdint>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache//ecs/world_filter.hpp>
#include <mustache/utils/dispatch.hpp>

namespace mustache {

    class Dispatcher;
    class Archetype;
    class World;

    class APerEntityJob {
    public:
        virtual ~APerEntityJob() = default;

        virtual void run(World&, Dispatcher&) = 0;

//        [[nodiscard]] virtual bool isArchetypeMatch(const Archetype& arch) const noexcept;

//        virtual uint32_t taskCount(uint32_t entities_count, uint32_t thread_count) const noexcept;

//        void applyFiler(World& world) noexcept;

        /*void runWithDefaultDispatcher(World& world) {
            run(world, getDispatcher());
        }*/
    };

    template<typename T/*, typename _WorldFilter = DefaultWorldFilterResult*/>
    class PerEntityJob : public APerEntityJob {
    public:
        using Info = JobInfo<T>;
        using WorldFilterResult = DefaultWorldFilterResult;//_WorldFilter;

        PerEntityJob() {
            filter_result_.mask_ = Info::componentMask();
        }

        MUSTACHE_INLINE void runParallel(World& world, uint32_t task_count, Dispatcher& dispatcher) {
            filter_result_.apply(world);
            const uint32_t ept = filter_result_.total_entity_count / task_count;
            const uint32_t num_archetypes = filter_result_.filtered_archetypes.size();
            const uint32_t tasks_with_extra_item = filter_result_.total_entity_count - task_count * ept;
            TaskArchetypeIndex first_a = TaskArchetypeIndex::make(0u);
            ArchetypeEntityIndex first_i = ArchetypeEntityIndex::make(0u);

            for(auto task = PerEntityJobTaskId::make(0); task < PerEntityJobTaskId::make(task_count); ++task) {
                const uint32_t current_task_size = task.toInt() < tasks_with_extra_item ? ept + 1 : ept;
                dispatcher.addParallelTask([first_a, first_i, task, current_task_size, this] {
                    constexpr auto index_sequence = std::make_index_sequence<Info::FunctionInfo::components_count>();
                    singleTask(task, first_a, first_i, current_task_size, index_sequence);
                });
                uint32_t num_entities_to_iterate = current_task_size;
                // iterate over archetypes and collect their entities until, ept was get
                while (first_a.toInt() < num_archetypes && task.toInt() != task_count - 1) {
                    const auto& archetype_info = filter_result_.filtered_archetypes[first_a.toInt()];
                    const uint32_t num_free_entities_in_arch = archetype_info.entities_count - first_i.toInt();
                    if(num_free_entities_in_arch > num_entities_to_iterate) {
                        first_i = ArchetypeEntityIndex::make(first_i.toInt() + num_entities_to_iterate);
                        break;
                    }
                    num_entities_to_iterate -= num_free_entities_in_arch;
                    first_a = TaskArchetypeIndex::make(first_a.toInt() + 1);
                    first_i = ArchetypeEntityIndex::make(0);
                }
            }
            dispatcher.waitForParallelFinish();
        }

        template<size_t... _I>
        MUSTACHE_INLINE void runSingleThread(World& world, const std::index_sequence<_I...>&) {
            static const auto mask = Info::componentMask();
            JobInvocationIndex invocation_index;
            invocation_index.task_index = PerEntityJobTaskId::make(0);
            invocation_index.entity_index_in_task = PerEntityJobEntityIndexInTask::make(0);
            world.entities().forEachArchetype([this, &invocation_index](Archetype& archetype) {
                if (archetype.isMatch(mask)) {
                    const auto size = ComponentArraySize::make(archetype.size());
                    constexpr auto entity_index = ArchetypeEntityIndex::make(0);
                    applyPerArrayFunction<_I...>(archetype, entity_index, size, invocation_index);

                    invocation_index.entity_index_in_task = PerEntityJobEntityIndexInTask::make(
                            invocation_index.entity_index_in_task.toInt() + archetype.size());
                }
            });
        }

        MUSTACHE_INLINE void run(World& world, Dispatcher&) override {
            runSingleThread(world, std::make_index_sequence<Info::FunctionInfo::components_count>());
        }

    protected:

        template<typename... _ARGS>
        MUSTACHE_INLINE void forEachArrayGenerated(ComponentArraySize count, JobInvocationIndex& invocation_index,
                                   _ARGS MUSTACHE_RESTRICT_PTR ... pointers) {
            T& self = *static_cast<T*>(this);
            if constexpr (Info::has_for_each_array) {
                invokeMethod(self, &T::forEachArray, count, invocation_index, pointers...);
            } else {
                for(uint32_t i = 0; i < count.toInt(); ++i) {
                    invoke(self, invocation_index, pointers++...);
                    ++invocation_index.entity_index_in_task;
                }
            }
        }

        template<size_t _I>
        MUSTACHE_INLINE auto getComponentHandler(const ComponentDataStorage::ElementView& view, ComponentIndex index) noexcept {
            using RequiredType = typename Info::FunctionInfo::template Component<_I>::type;
            using Component = typename ComponentType<RequiredType>::type;
            if constexpr (IsComponentRequired<RequiredType>::value) {
                auto ptr = view.getData<FunctionSafety::kUnsafe>(index);
                return RequiredComponent<Component> {reinterpret_cast<Component*>(ptr)};
            } else {
                // TODO: it is possible to avoid per array check.
                auto ptr = view.getData<FunctionSafety::kSafe>(index);
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
                forEachArrayGenerated(ComponentArraySize::make(arr_len), invocation_index,
                        view.getEntity<FunctionSafety::kUnsafe>(), getComponentHandler<_I>(view, component_indexes[_I])...);
                view.add(arr_len);
                elements_rest -= arr_len;
            }
        }

        /// NOTE: world filtering required
        template<size_t... _I>
        MUSTACHE_INLINE void singleTask(PerEntityJobTaskId task_id, TaskArchetypeIndex archetype_index,
                                        ArchetypeEntityIndex begin, uint32_t count, const std::index_sequence<_I...>&) {
            const uint32_t num_archetypes = filter_result_.filtered_archetypes.size();
            JobInvocationIndex invocation_index;
            invocation_index.task_index = task_id;
            invocation_index.entity_index_in_task = PerEntityJobEntityIndexInTask::make(0);
            while (count != 0 && archetype_index.toInt() < num_archetypes) {
                auto& archetype_info = filter_result_.filtered_archetypes[archetype_index.toInt()];
                const uint32_t num_free_entities_in_arch = archetype_info.entities_count - begin.toInt();
                const uint32_t objects_to_iterate = count < num_free_entities_in_arch ? count : num_free_entities_in_arch;
                const auto array_size = ComponentArraySize::make(objects_to_iterate);
                applyPerArrayFunction<_I...>(*archetype_info.archetype, begin, array_size, invocation_index);
                count -= objects_to_iterate;
                begin = ArchetypeEntityIndex::make(0);
                ++archetype_index;
            }
            if(count > 0) {
                throw std::runtime_error(std::to_string(count) + " entities were not updated!");
            }
        }

        WorldFilterResult filter_result_;
    };

}