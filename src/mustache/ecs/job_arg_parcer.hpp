#pragma once

#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_handler.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/entity.hpp>

#include <mustache/utils/invoke.hpp>
#include <mustache/utils/dispatch.hpp>
#include <mustache/utils/type_info.hpp>
#include <mustache/utils/function_traits.hpp>

#include <array>
#include <iostream>
#include <cstdint>

namespace mustache {

    struct JobInvocationIndex {
        PerEntityJobTaskId task_index;
        PerEntityJobEntityIndexInTask entity_index_in_task;
        ThreadId thread_id;
    };

    template <typename T>
    struct IsArgEntity {
        static constexpr bool value = IsOneOfTypes<T, Entity, const Entity&, const Entity*>::value;
        static_assert(!IsOneOfTypes<T, Entity&, Entity*>::value, "non const entity references/pointers are no allowed");
    };

    template <typename T>
    struct IsArgJobInvocationIndex {
        static constexpr bool value = IsOneOfTypes<T, JobInvocationIndex, const JobInvocationIndex&>::value;
    };

    template <typename T>
    struct IsArgComponentArraySize {
        static constexpr bool value = IsOneOfTypes<T, ComponentArraySize, const ComponentArraySize&>::value;
    };

    template <typename Element, typename... ARGS>
    constexpr int32_t elementIndex(Element&& element, ARGS&&... args) {
        const std::array<bool, sizeof...(ARGS)> is_arg_match {
                (element == args)...
        };
        for (size_t i = 0u; i < is_arg_match.size(); ++i) {
            if (is_arg_match[i]) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    template <typename... ARGS>
    struct PositionOf {
        static constexpr int32_t entity = elementIndex(true, IsArgEntity<ARGS>::value...);
        static constexpr int32_t job_invocation = elementIndex(true, IsArgJobInvocationIndex<ARGS>::value...);
        static constexpr int32_t array_size = elementIndex(true, IsArgComponentArraySize<ARGS>::value...);
    };

    template <typename T>
    class JobFunctionInfo {
    public:
        struct ArgInfo {
           enum ArgType: uint32_t {
               kComponent = 0,
               kSharedComponent = 1,
               kEntity = 2,
               kInvocationIndex = 3
           };
           ArgType type;
           uint32_t position;
            constexpr ArgInfo() = default;
            constexpr ArgInfo(ArgType t, uint32_t pos):
                type {t},
                position {pos} {
            }
        };
        template<size_t _I>
        static constexpr ArgInfo getArgInfo() noexcept {
            using ArgType = typename utils::function_traits<T>::template arg<_I>::type;
            if constexpr (IsArgEntity<ArgType>::value)  {
                return ArgInfo(ArgInfo::kEntity, _I);
            }
            if constexpr (IsArgJobInvocationIndex<ArgType>::value) {
                return ArgInfo(ArgInfo::kInvocationIndex, _I);
            }
            if constexpr (isComponentShared<ArgType>()) {
                return ArgInfo(ArgInfo::kSharedComponent, _I);
            }
            return ArgInfo(ArgInfo::kComponent, _I);
        }

        template<size_t... _I>
        static constexpr std::array<ArgInfo, sizeof...(_I)> buildArgsInfo(const std::index_sequence<_I...>&) {
            return std::array<ArgInfo, sizeof...(_I)> {
                getArgInfo<_I>()...
            };
        }
        using FunctionType = T;
        using FC = utils::function_traits<T>;
        static constexpr size_t args_count = FC::arity;
        static constexpr auto args_indexes = std::make_index_sequence<args_count>();
        static constexpr auto args_info = buildArgsInfo(args_indexes);
    private:
        template<size_t... _I>
        static constexpr auto buildPositionsOf(const std::index_sequence<_I...>&) {
            return PositionOf<typename FC::template arg<_I>::type...>{};
        }
    public:
        using Position = decltype(buildPositionsOf(args_indexes));

        template<size_t... _I>
        static constexpr size_t sharedComponentsCount(const std::index_sequence<_I...>&) noexcept {
            const auto count_of = [](bool v) {
                return v ? 1 : 0;
            };
            return (count_of(isComponentShared<typename FC::template arg<_I>::type>()) + ... + 0);
        }

        static constexpr size_t sharedComponentsCount() noexcept {
            return sharedComponentsCount(args_indexes);
        }

        static constexpr size_t anyComponentsCount() noexcept {
            size_t result = 0;
            for (const auto& info : args_info) {
                if (info.type == ArgInfo::kSharedComponent || info.type == ArgInfo::kComponent) {
                    ++result;
                }
            }
            return result;
        }

        static constexpr size_t componentsCount() noexcept {
            return anyComponentsCount() - sharedComponentsCount();
        }

        static constexpr size_t any_components_count = anyComponentsCount();
        static constexpr size_t components_count = componentsCount();
        static constexpr size_t shared_components_count = sharedComponentsCount();


        static constexpr size_t componentPosition(size_t component_index) noexcept {
            uint32_t result = 0u;
            for (const auto& info : args_info) {
                if (info.type == ArgInfo::kSharedComponent || info.type == ArgInfo::kComponent) {
                    if (component_index < 1) {
                        break;
                    }
                    --component_index;
                }
                ++result;
            }
            return result;
        }

        static constexpr size_t uniqueComponentPosition(size_t component_index) noexcept {
            uint32_t result = 0u;
            for (const auto& info : args_info) {
                if (info.type == ArgInfo::kComponent) {
                    if (component_index < 1) {
                        break;
                    }
                    --component_index;
                }
                ++result;
            }
            return result;
        }
        static constexpr size_t sharedComponentPosition(size_t component_index) noexcept {
            uint32_t result = 0u;
            for (const auto& info : args_info) {
                if (info.type == ArgInfo::kSharedComponent) {
                    if (component_index < 1) {
                        break;
                    }
                    --component_index;
                }
                ++result;
            }
            return result;
        }

        template<size_t... _I>
        static constexpr auto buildComponentIndexes(const std::index_sequence<_I...>&) {
            return std::index_sequence<componentPosition(_I)...>{};
        }
        template<size_t... _I>
        static constexpr auto buildSharedComponentIndexes(const std::index_sequence<_I...>&) {
            return std::index_sequence<sharedComponentPosition(_I)...>{};
        }
        template<size_t... _I>
        static constexpr auto buildUniqueComponentIndexes(const std::index_sequence<_I...>&) {
            return std::index_sequence<componentPosition(_I)...>{};
        }

        template<size_t... _I>
        static constexpr auto buildArgsTuple(const std::index_sequence<_I...>&) {
            struct ReturnType {
                using type [[maybe_unused]] = std::tuple< typename FC::template arg<_I>::type...>;
            };
            return ReturnType{}; // tuple type may be non-default constructable
        }
        static constexpr auto any_components_indexes =
                buildComponentIndexes(std::make_index_sequence<any_components_count>());
        static constexpr auto shared_components_indexes =
                buildSharedComponentIndexes(std::make_index_sequence<shared_components_count>());
        static constexpr auto unique_components_indexes =
                buildUniqueComponentIndexes(std::make_index_sequence<components_count>());

        using ArgsTypeTuple = typename decltype(buildArgsTuple(args_indexes))::type;
        using AnyComponentsTypeTuple = typename decltype(buildArgsTuple(any_components_indexes))::type;
        using UniqueComponentsTypeTuple = typename decltype(buildArgsTuple(unique_components_indexes))::type;
        using SharedComponentsTypeTuple = typename decltype(buildArgsTuple(shared_components_indexes))::type;

        template<size_t I>
        struct AnyComponentType {
            using type = typename std::tuple_element<I, AnyComponentsTypeTuple>::type;
        };
        template<size_t I>
        struct UniqueComponentType {
            using type = typename std::tuple_element<I, UniqueComponentsTypeTuple>::type;
        };
        template<size_t I>
        struct SharedComponentType {
            using type = typename std::tuple_element<I, SharedComponentsTypeTuple>::type;
        };
    };

    template<typename T>
    struct JobInfo {
    private:
        template<typename C>
        static constexpr bool testForEachArray(decltype(&C::forEachArray)) noexcept {
            return true;
        }

        template<typename C>
        static constexpr bool testForEachArray(...) noexcept {
            return false;
        }
        template<typename C>
        static constexpr bool testCallOperator(decltype(&C::operator())) noexcept {
            return true;
        }

        template<typename C>
        static constexpr bool testCallOperator(...) noexcept {
            return false;
        }

        static constexpr auto getJobFunctionInfo() noexcept {
            if constexpr (testForEachArray<T>(nullptr)) {
                return JobFunctionInfo<decltype(&T::forEachArray)>{};
            } else {
                return JobFunctionInfo<T>{};
            }
        }
        
    public:
        JobInfo() = delete;
        ~JobInfo() = delete;

        static_assert(testForEachArray<T>(nullptr) || testCallOperator<T>(nullptr),
                "T is not a job type, operator() or method forEachArray does not found");
        using FunctionInfo = decltype(getJobFunctionInfo());
        static constexpr bool has_for_each_array = testForEachArray<T>(nullptr);

        template<size_t... _I>
        static ComponentIdMask componentMask(const std::index_sequence<_I...>&) noexcept {
            return ComponentFactory::makeMask<typename FunctionInfo::template UniqueComponentType<_I> ::type...>();
        }
        static ComponentIdMask componentMask() noexcept {
            return componentMask(std::make_index_sequence<FunctionInfo::components_count>());
        }

        template<typename _C>
        static std::pair<ComponentId, bool> componentInfo() noexcept {
            using Component = typename ComponentType<_C>::type;
            return std::make_pair(ComponentFactory::registerComponent<Component>(),
                    IsComponentMutable<_C>::value);
        }

        template<size_t... _I>
        static ComponentIdMask updateMask(std::index_sequence<_I...>&&) noexcept {
            ComponentIdMask result;
            std::array array {
                componentInfo<typename FunctionInfo::template UniqueComponentType<_I> ::type>()...
            };
            for (const auto& pair : array) {
                result.set(pair.first, pair.second);
            }
            return result;
        }
        static ComponentIdMask updateMask() noexcept {
            if constexpr (FunctionInfo::components_count < 1) {
                return ComponentIdMask{};
            } else {
                return updateMask(std::make_index_sequence<FunctionInfo::components_count>());
            }
        }
    };

    template<typename T, size_t... _I>
    void showJobInfo(const std::index_sequence<_I...>&) {
        using info = typename JobInfo<T>::FunctionInfo;
        std::cout << "Job type name: " << type_name<T>() << std::endl;
        std::cout << "\tFunctor type: " << type_name<typename info::FunctionType>() << std::endl;
        std::cout << "\tArgs count: " << info::args_count << std::endl;
        std::cout << "\tComponents count: " << info::components_count << std::endl;
        const auto show_component = [](auto pairs){
            for (const auto pair : pairs) {
                std::cout << "\tComponent name: " << pair.second << ", position: " << pair.first << std::endl;
            }
        };
        if constexpr (sizeof...(_I) > 0) {
            const std::array array = {std::make_pair(info::componentPosition(_I),
                                      type_name<typename info::template UniqueComponentType<_I>::type>())...};
            show_component(array);
        }
    }

    template<typename T>
    void showJobInfo() {
        using info = typename JobInfo<T>::FunctionInfo;
        showJobInfo<T>(std::make_index_sequence<info::components_count>());
    }
}
