#pragma once

#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_handler.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/entity.hpp>

#include <mustache/utils/type_info.hpp>
#include <mustache/utils/function_traits.hpp>
#include <mustache/utils/dispatch.hpp>

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

    template<size_t I, typename T, typename... ARGS>
    constexpr decltype(auto) selectArgByIndex(T&& first, ARGS&&... rest) {
        if constexpr (I == 0) {
            return std::forward<T>(first);
        } else {
            return selectArgByIndex<I - 1>(std::forward<ARGS>(rest)...);
        }
    }

    template<typename TargetType, typename T, typename... ARGS>
    constexpr decltype(auto) selectArgByType(T&& first, ARGS&&... rest) {
        if constexpr (std::is_convertible<T, TargetType>::value) { // TODO: check if it is enough
            return std::forward<T>(first);
        } else {
            return selectArgByType<TargetType>(std::forward<ARGS>(rest)...);
        }
    }

    template<typename F, size_t... I, typename... ARGS>
    MUSTACHE_INLINE auto invokeWithIndexSequence(F&& func, const std::index_sequence<I...>&, ARGS&&... args) {
        using FC = utils::function_traits<F>;
        return func(selectArgByType<typename FC::template arg<I>::type>(std::forward<ARGS>(args)...)...);
    }
    template<typename F, typename... ARGS>
    MUSTACHE_INLINE auto invoke(F&& func, ARGS&&... args) {
        using FC = utils::function_traits<F>;
        return invokeWithIndexSequence(std::forward<F>(func),
                std::make_index_sequence<FC::arity>(), std::forward<ARGS>(args)...);
    }

    template<typename T, typename F, size_t... I, typename... ARGS>
    auto invokeMethodWithIndexSequence(T& object, F&& method, const std::index_sequence<I...>&, ARGS&&... args) {
        using FC = utils::function_traits<F>;
        return (object.*method)(selectArgByType<typename FC::template arg<I>::type>(std::forward<ARGS>(args)...)...);
    }

    template<typename T, typename F, typename... ARGS>
    auto invokeMethod(T& object, F&& method, ARGS&&... args) {
        using FC = utils::function_traits<F>;
        return invokeMethodWithIndexSequence(object, std::forward<F>(method),
                                       std::make_index_sequence<FC::arity>(), std::forward<ARGS>(args)...);
    }

    template <typename T>
    class JobFunctionInfo {
    public:
        using FunctionType = T;
        using FC = utils::function_traits<T>;
        static constexpr size_t args_count = FC::arity;
        static constexpr auto index_sequence = std::make_index_sequence<args_count>();
    private:
        template<size_t... _I>
        static constexpr auto buildPositionsOf(const std::index_sequence<_I...>&) {
            return PositionOf<typename FC::template arg<_I>::type...>{};
        }
    public:
        using Position = decltype(buildPositionsOf(index_sequence));

        static constexpr size_t componentsCount() noexcept {
            size_t result = args_count;
            if constexpr (Position::entity >= 0) {
                --result;
            }
            if constexpr (Position::job_invocation >= 0) {
                --result;
            }
            if constexpr (Position::array_size >= 0) {
                --result;
            }

            return result;
        }

        static constexpr size_t components_count = componentsCount();


        static constexpr size_t componentPosition(size_t component_index) noexcept {
            if (component_index >= static_cast<size_t>(Position::entity)) {
                ++component_index;
            }
            if (component_index >= static_cast<size_t>(Position::job_invocation)) {
                ++component_index;
            }
            if (component_index >= static_cast<size_t>(Position::array_size)) {
                ++component_index;
            }
            return component_index;
        }

        template<size_t... _I>
        static constexpr auto buildComponentIndexes(const std::index_sequence<_I...>&) {
            return std::index_sequence<componentPosition(_I)...>{};
        }

        template<size_t... _I>
        static constexpr auto buildArgsTuple(const std::index_sequence<_I...>&) {
            struct ReturnType {
                using type [[maybe_unused]] = std::tuple< typename FC::template arg<_I>::type...>;
            };
            return ReturnType{}; // tuple type may be non-default constructable
        }
        static constexpr auto components_positions = buildComponentIndexes(std::make_index_sequence<components_count>());
        using ArgsTypeTuple = typename decltype(buildArgsTuple(index_sequence))::type;
        using ComponentsTypeTuple = typename decltype(buildArgsTuple(components_positions))::type;

        template<size_t I>
        struct Component {
            using type = typename std::tuple_element<I, ComponentsTypeTuple>::type;
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
        using FunctionInfo = decltype(getJobFunctionInfo());
        static constexpr bool has_for_each_array = testForEachArray<T>(nullptr);

        template<size_t... _I>
        static ComponentIdMask componentMask(std::index_sequence<_I...>&&) noexcept {
            return ComponentFactory::makeMask<typename FunctionInfo::template Component<_I> ::type...>();
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
                componentInfo<typename FunctionInfo::template Component<_I> ::type>()...
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
                                                     type_name<typename info::template Component<_I>::type>())...};
            show_component(array);
        }
    }

    template<typename T>
    void showJobInfo() {
        using info = typename JobInfo<T>::FunctionInfo;
        showJobInfo<T>(std::make_index_sequence<info::components_count>());
    }
}