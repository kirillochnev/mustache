#pragma once

#include <mustache/ecs/shared_component.hpp>

#include <stdexcept>
#include <type_traits>

namespace mustache {

    template<typename T>
    using Component = std::conditional_t< std::is_const_v<T>,
            std::conditional_t< std::is_trivially_copyable_v<T> && sizeof(T) <= 2 * sizeof(void*),
                    const T,
                    const T&
            >,
            T&
    >;

    template <typename T, typename... ARGS>
    struct IsOneOfTypes {
        static constexpr bool value = (std::is_same<T, ARGS>::value || ...);
    };

    template<typename T, bool _IsRequired>
    class ComponentHandler {
    public:
        ComponentHandler() = default;

        ComponentHandler(T& value):
                ptr_{&value} {

        }
        ComponentHandler(T* value):
                ptr_{value} {

        }

        decltype(auto) operator[](size_t i) const noexcept {
            if constexpr(_IsRequired) {
                return ptr_[i];
            } else {
                return ComponentHandler{ptr_ + i * (ptr_ != nullptr)};
            }
        }

        ComponentHandler operator+=(size_t count) {
            if constexpr (std::is_base_of<SharedComponentTag, T>::value) {
                return *this;
            } else {
                ComponentHandler cpy = *this;
                if constexpr (_IsRequired) {
                    ptr_ += count;
                } else {
                    if (ptr_ != nullptr) {
                        ptr_ += count;
                    }
                }
                return cpy;
            }
        }

        ComponentHandler operator++(int) {
            return *this+=1;
        }

        operator bool() const noexcept {
            return ptr_ != nullptr;
        }
        bool operator==(const std::nullptr_t&) const noexcept {
            return ptr_ == nullptr;
        }
        bool operator!=(const std::nullptr_t&) const noexcept {
            return ptr_ != nullptr;
        }
        operator T&() noexcept {
            return *ptr_;
        }
        operator T*() noexcept {
            return ptr_;
        }

        operator const T&() const noexcept {
            return *ptr_;
        }
        operator const T*() const noexcept {
            return ptr_;
        }
        T* get() noexcept {
            return ptr_;
        }
        const T* get() const noexcept {
            return ptr_;
        }
        const T* operator->() const noexcept {
            return ptr_;
        }
        T* operator->() noexcept {
            return ptr_;
        }
        const T& operator*() const noexcept(_IsRequired) {
            if constexpr (!_IsRequired) {
                if (ptr_ == nullptr) {
                    throw std::runtime_error("null component handler dereference");
                }
            }
            return *ptr_;
        }
        T& operator*() noexcept(_IsRequired) {
            if constexpr (!_IsRequired) {
                if (ptr_ == nullptr) {
                    throw std::runtime_error("null component handler dereference");
                }
            }
            return *ptr_;
        }

    private:
        T* ptr_ = nullptr;
    };

    template<typename T>
    using RequiredComponent = ComponentHandler<T, true>;


    template<typename T>
    using OptionalComponent = ComponentHandler<T, false>;


    template<typename T>
    using SharedComponent = ComponentHandler<T, true>;

    template<typename T>
    struct IsComponentMutable {
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsComponentMutable<const T*> {
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsComponentMutable<const T* const> {
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsComponentMutable<const T&> {
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsComponentMutable<const RequiredComponent<const T>&> {
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsComponentMutable<RequiredComponent<const T> > {
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsComponentMutable<const OptionalComponent<const T>&> {
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsComponentMutable<OptionalComponent<const T> > {
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


    template <typename T>
    constexpr bool isComponentShared() noexcept {
        using PureType = typename ComponentType<T>::type;
        return std::is_base_of<SharedComponentTag, PureType>::value;
    }

    template <typename T>
    struct IsComponentRequired {
        using Component = typename ComponentType<T>::type;
        static constexpr bool value = IsOneOfTypes<T,
                Component, Component&, const Component&, const RequiredComponent<Component>&,
                RequiredComponent<Component>, RequiredComponent<const Component> >::value;
    };

    template <typename T>
    struct IsComponentOptional {
        using Component = typename ComponentType<T>::type;
        static constexpr bool value = IsOneOfTypes<T, Component*, const Component*,
                OptionalComponent<Component>, OptionalComponent<const Component> >::value;
    };
}
