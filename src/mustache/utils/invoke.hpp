#pragma once

#include <mustache/utils/function_traits.hpp>

namespace mustache {

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
}
