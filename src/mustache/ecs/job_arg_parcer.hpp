#pragma once

#include <mustache/utils/invoke.hpp>
#include <mustache/utils/dispatch.hpp>
#include <mustache/utils/type_info.hpp>
#include <mustache/utils/function_traits.hpp>

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_handler.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <array>
#include <cstdint>
#include <type_traits>

namespace mustache {

    class World;

    struct MUSTACHE_EXPORT JobInvocationIndex {
        ParallelTaskId task_index;
        ParallelTaskItemIndexInTask entity_index_in_task;
        ParallelTaskGlobalItemIndex entity_index;
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

    template <typename T>
    struct IsArgWorld {
        static constexpr bool value = IsOneOfTypes<T, World&, const World&>::value;
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
                kInvocationIndex = 3,
                kArraySize = 4,
                kWorld = 5
            };
            ArgType type;
            uint32_t position;
            bool is_mutable;
            constexpr ArgInfo() = default;
            constexpr ArgInfo(ArgType t, uint32_t pos, bool _is_mutable):
                    type {t},
                    position {pos},
                    is_mutable{_is_mutable} {
            }
        };
        template<size_t _I>
        static constexpr ArgInfo getArgInfo() noexcept {
            using ArgType = typename utils::function_traits<T>::template arg<_I>::type;
            if constexpr (IsArgEntity<ArgType>::value)  {
                return ArgInfo(ArgInfo::kEntity, _I, false);
            }
            if constexpr (IsArgJobInvocationIndex<ArgType>::value) {
                return ArgInfo(ArgInfo::kInvocationIndex, _I, false);
            }
            if constexpr(IsArgComponentArraySize<ArgType>::value) {
                return ArgInfo(ArgInfo::kArraySize, _I, false);
            }
            if constexpr(IsArgWorld<ArgType>::value) {
                return ArgInfo(ArgInfo::kWorld, _I, true);
            }

            // Arg is component
            if constexpr (isComponentShared<ArgType>()) {
                return ArgInfo(ArgInfo::kSharedComponent, _I, false);
            }

            return ArgInfo(ArgInfo::kComponent, _I, IsComponentMutable<ArgType>::value);
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

        template <typename Tuple, std::size_t... I>
        static constexpr std::array<std::size_t, sizeof...(I)>
        tupleElementSizes(std::index_sequence<I...>) noexcept {
            return { sizeof(std::tuple_element_t<I, Tuple>)... };
        }

        template <std::size_t... I>
        static constexpr std::size_t maxUniqueComponentSizeImpl(std::index_sequence<I...>) noexcept {
            auto sizes = tupleElementSizes<UniqueComponentsTypeTuple>(std::index_sequence<I...>{});
            std::size_t m = 0;
            for (auto s : sizes) {
                if (s > m) m = s;
            }
            return m;
        }

        template <std::size_t... I>
        static constexpr std::size_t totalUniqueComponentsSizeImpl(std::index_sequence<I...>) noexcept {
            auto sizes = tupleElementSizes<UniqueComponentsTypeTuple>(std::index_sequence<I...>{});
            std::size_t sum = 0;
            for (auto s : sizes) {
                sum += s;
            }
            return sum;
        }

    public:
        using Position = decltype(buildPositionsOf(args_indexes));

        static constexpr size_t sharedComponentsCount() noexcept {
            size_t result = 0;
            for (const auto& info : args_info) {
                if (info.type == ArgInfo::kSharedComponent) {
                    ++result;
                }
            }
            return result;
        }

        static constexpr size_t mutableComponentsCount() noexcept {
            size_t result = 0;
            for (const auto& info : args_info) {
                if (info.type == ArgInfo::kComponent && info.is_mutable) {
                    ++result;
                }
            }
            return result;
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
            size_t result = 0;
            for (const auto& info : args_info) {
                if (info.type == ArgInfo::kComponent) {
                    ++result;
                }
            }
            return result;
        }

        static constexpr size_t any_components_count = anyComponentsCount();
        static constexpr size_t mutable_components_count = mutableComponentsCount();
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
        static constexpr size_t mutableComponentPosition(size_t component_index) noexcept {
            uint32_t result = 0u;
            for (const auto& info : args_info) {
                if (info.type == ArgInfo::kComponent && info.is_mutable) {
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
        static constexpr auto buildMutableComponentIndexes(const std::index_sequence<_I...>&) {
            return std::index_sequence<mutableComponentPosition(_I)...>{};
        }
        template<size_t... _I>
        static constexpr auto buildUniqueComponentIndexes(const std::index_sequence<_I...>&) {
            return std::index_sequence<uniqueComponentPosition(_I)...>{};
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
        static constexpr auto mutable_components_indexes =
                buildMutableComponentIndexes(std::make_index_sequence<mutable_components_count>());
        static constexpr auto unique_components_indexes =
                buildUniqueComponentIndexes(std::make_index_sequence<components_count>());

        using ArgsTypeTuple = typename decltype(buildArgsTuple(args_indexes))::type;
        using AnyComponentsTypeTuple = typename decltype(buildArgsTuple(any_components_indexes))::type;
        using UniqueComponentsTypeTuple = typename decltype(buildArgsTuple(unique_components_indexes))::type;
        using SharedComponentsTypeTuple = typename decltype(buildArgsTuple(shared_components_indexes))::type;

        [[nodiscard]] static constexpr std::size_t maxUniqueComponentSize() noexcept {
            if constexpr (components_count == 0) {
                return 0;
            } else {
                return maxUniqueComponentSizeImpl(std::make_index_sequence<components_count>{});
            }
        }

        [[nodiscard]] static constexpr std::size_t totalUniqueComponentsSize() noexcept {
            if constexpr (components_count == 0) {
                return 0;
            } else {
                return totalUniqueComponentsSizeImpl(std::make_index_sequence<components_count>{});
            }
        }

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

        static constexpr bool isNoexcept() noexcept {
            if constexpr (testForEachArray<T>(nullptr)) {
                return noexcept(&T::forEachArray);
            }
            if constexpr (testCallOperator<T>(nullptr)) {
                return noexcept(&T::operator());
            }
            return false;
        }
        static constexpr bool isConstThis() noexcept {
            if constexpr (testForEachArray<T>(nullptr)) {
                return checkConst(&T::forEachArray);
            }
            if constexpr (testCallOperator<T>(nullptr)) {
                return checkConst(&T::operator());
            }
            return true;
        }

        template <typename C, typename R, typename... A>
        static constexpr bool checkConst(R (C::*)(A...) const ) {
            return true;
        }
        template <typename C, typename R, typename... A>
        static constexpr bool checkConst(R (C::*)(A...) ) {
            return false;
        }

    public:
        JobInfo() = delete;
        ~JobInfo() = delete;

//        static_assert(testForEachArray<T>(nullptr) || testCallOperator<T>(nullptr),
//                      "T is not a job type, operator() or method forEachArray does not found");


        static constexpr bool has_for_each_array = testForEachArray<T>(nullptr);
        static constexpr bool is_noexcept = isNoexcept();
        static constexpr bool is_const_this = isConstThis();

        using FunctionInfo = decltype(getJobFunctionInfo());

        template<size_t... _I>
        static ComponentIdMask componentMask(const std::index_sequence<_I...>&) noexcept {
            return ComponentFactory::instance().makeMask<typename FunctionInfo::template UniqueComponentType<_I> ::type...>();
        }
        static ComponentIdMask componentMask() noexcept {
            return componentMask(std::make_index_sequence<FunctionInfo::components_count>());
        }

        template<size_t... _I>
        static SharedComponentIdMask sharedComponentMask(const std::index_sequence<_I...>&) noexcept {
            return ComponentFactory::instance().makeSharedMask<typename FunctionInfo::template SharedComponentType<_I> ::type...>();
        }
        static SharedComponentIdMask sharedComponentMask() noexcept {
            return sharedComponentMask(std::make_index_sequence<FunctionInfo::shared_components_count>());
        }

        template<typename _C>
        static std::pair<ComponentId, bool> componentInfo() noexcept {
            using Component = typename ComponentType<_C>::type;
            return std::make_pair(ComponentFactory::instance().registerComponent<Component>(),
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
                return ComponentIdMask::null();
            } else {
                return updateMask(std::make_index_sequence<FunctionInfo::components_count>());
            }
        }
    };

}
