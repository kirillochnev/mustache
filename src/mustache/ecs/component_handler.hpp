#pragma once

namespace mustache {

    template <typename T, typename... ARGS>
    struct IsOneOfTypes {
        static constexpr bool value = (std::is_same<T, ARGS>::value || ...);
    };

    template <typename T>
    struct RequiredComponent {
        T* ptr;
        RequiredComponent() = default;

        RequiredComponent(T& value):
            ptr{&value} {

        }
        RequiredComponent(T* value):
                ptr{value} {

        }
        operator T&() {
            return *ptr;
        }
        operator T*() {
            return ptr;
        }

        operator const T&() const {
            return *ptr;
        }
        operator const T*() const {
            return ptr;
        }

    };

    template <typename T>
    struct OptionalComponent {
        T* ptr;
    };

    template <typename T>
    struct IsComponentOptional {
        static constexpr bool value = IsOneOfTypes<T*, const T*,
                OptionalComponent<T>, OptionalComponent<const T> >::value;
    };

    template <typename T>
    struct IsComponentRequired {
        static constexpr bool value = IsOneOfTypes<T&, const T&, const RequiredComponent<T>&,
                RequiredComponent<T>, RequiredComponent<const T> >::value;
    };

    template<typename T>
    struct IsComponentMutable {
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsComponentMutable<T*> {
        constexpr static bool value = true;
    };

    template<typename T>
    struct IsComponentMutable<T* const> {
        constexpr static bool value = true;
    };

    template<typename T>
    struct IsComponentMutable<T&> {
        constexpr static bool value = true;
    };


    template<typename T>
    struct ComponentType {
        using type = typename std::remove_const<typename std::remove_pointer<T>::type>::type;
        constexpr static bool is_component_required = std::is_same<const T, const type >::value;
        constexpr static bool is_component_mutable = IsComponentMutable<T>::value;
    };

    template<typename T>
    struct ComponentType<T&> {
        using type = typename std::remove_const<T>::type;
        constexpr static bool is_component_required = true;
        constexpr static bool is_component_mutable = IsComponentMutable<T&>::value;
    };

    template<typename T>
    struct ComponentType<RequiredComponent<T> > {
        static_assert(std::is_same<T, typename ComponentType<T>::type>::value, "RequiredComponent<T>. T can not have any other qualifiers");

        using type = typename std::remove_const<T>::type ;
        constexpr static bool is_component_required = true;
        constexpr static bool is_component_mutable = IsComponentMutable<T>::value;
    };

    template<typename T>
    struct ComponentType<const RequiredComponent<T> > {
        static_assert(std::is_same<T, typename ComponentType<T>::type>::value, "const RequiredComponent<T>. T can not have any other qualifiers");

        using type = typename std::remove_const<T>::type ;
        constexpr static bool is_component_required = true;
        constexpr static bool is_component_mutable = false;
    };

    template<typename T>
    struct ComponentType<RequiredComponent<T>& > {
        static_assert(!std::is_reference<RequiredComponent<T>&>::value, "RequiredComponent non-const reference is not allowed");
    };

    template<typename T>
    struct ComponentType<const RequiredComponent<T>& > {
        static_assert(std::is_same<T, typename ComponentType<T>::type>::value, "const RequiredComponent<T>&. T can not have any other qualifiers");
        using type = typename std::remove_const<T>::type ;
        constexpr static bool is_component_required = true;
        constexpr static bool is_component_mutable = false;
    };

    template<typename T>
    struct ComponentType<OptionalComponent<T> > {
        static_assert(std::is_same<T, typename ComponentType<T>::type>::value, "OptionalComponent<T>. T can not have any other qualifiers");

        using type = typename std::remove_const<T>::type ;
        constexpr static bool is_component_required = false;
        constexpr static bool is_component_mutable = IsComponentMutable<T>::value;
    };

    template<typename T>
    struct ComponentType<const OptionalComponent<T> > {
        static_assert(std::is_same<T, typename ComponentType<T>::type>::value, "const OptionalComponent<T>. T can not have any other qualifiers");

        using type = typename std::remove_const<T>::type ;
        constexpr static bool is_component_required = false;
        constexpr static bool is_component_mutable = false;
    };

    template<typename T>
    struct ComponentType<OptionalComponent<T>& > {
        static_assert(!std::is_reference<RequiredComponent<T>&>::value, "OptionalComponent non-const reference is not allowed");
    };

    template<typename T>
    struct ComponentType<const OptionalComponent<T>& > {
        static_assert(std::is_same<T, typename ComponentType<T>::type>::value, "const OptionalComponent<T>&. T can not have any other qualifiers");
        using type = typename std::remove_const<T>::type ;
        constexpr static bool is_component_required = false;
        constexpr static bool is_component_mutable = false;
    };



}