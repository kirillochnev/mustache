#pragma once

#include <mustache/utils/dll_export.h>
#include <mustache/utils/type_info.hpp>

#include <functional>
#include <string>
#include <cstddef>

namespace mustache {
    namespace detail {
        template<typename C>
        inline constexpr bool testOperatorEq(decltype(&C::operator==)) noexcept {
            return true;
        }

        template<typename C>
        inline constexpr bool testOperatorEq(...) noexcept {
            return false;
        }
    }


    class World;
    struct Entity;

    template<typename _Sign>
    using Functor = std::function<_Sign>;

    struct MUSTACHE_EXPORT ComponentInfo {
        using Constructor = Functor<void (void*, const Entity&, World&) >;
        using CopyFunction = Functor<void (void*, const void*) >;
        using MoveFunction = Functor<void (void*, void*) >;
        using Destructor = Functor<void (void*/*, const Entity&, World&*/) >;
        using IsEqual = Functor<bool (const void*, const void*)>;

        size_t size{0};
        size_t align{0};
        std::string name;
        size_t type_id_hash_code;
        struct FunctionSet {
            Constructor create;
            CopyFunction copy;
            MoveFunction move;
            MoveFunction move_constructor;
            Destructor destroy;
            IsEqual compare;
        } functions;

        std::vector<std::byte> default_value; // this array will be used to init component in case of empty constructor

        template<typename T>
        static void componentConstructor(void *ptr, [[maybe_unused]] const Entity& entity, [[maybe_unused]] World& world) {
            if constexpr(std::is_constructible<T, World&, Entity>::value) {
                new(ptr) T {world, entity};
                return;
            }
            if constexpr(std::is_constructible<T, Entity, World&>::value) {
                new(ptr) T {entity, world};
                return;
            }
            if constexpr(std::is_constructible<T, World&>::value) {
                new(ptr) T {world};
                return;
            }
            if constexpr(std::is_constructible<T, const Entity&>::value) {
                new(ptr) T {entity};
                return;
            }
            if constexpr(std::is_default_constructible<T>::value &&
                        !std::is_trivially_default_constructible<T>::value) {
                new(ptr) T();
                return;
            }
        }

        template<typename T>
        static void componentDestructor(void* ptr/*, [[maybe_unused]] const Entity& entity, [[maybe_unused]] World& world*/) {
            static_cast<T*>(ptr)->~T();
        }
        template<typename T>
        static void copyComponent([[maybe_unused]] void* dest, [[maybe_unused]] const void* source) {
            if constexpr (std::is_copy_constructible_v<T>) {
                new(dest) T{ *static_cast<const T*>(source) };
            }
            else {
                error(type_name<T>() + " is not copy constructible");
            }
        }

        template<typename T>
        static void moveComponent(void* dest, void* source) {
            *static_cast<T*>(dest) = std::move(*static_cast<T*>(source));
        }    

        template<typename T>
        static void componentMoveConstructor([[maybe_unused]] void* dest, [[maybe_unused]] void* source) {
            if constexpr (std::is_move_constructible_v<T>) {
                new(dest) T{ std::move(*static_cast<T*>(source)) };
            }
            else {
                if constexpr (std::is_default_constructible_v<T>) {
                    T* ptr = new(dest) T{};
                    *std::launder(ptr) = std::move(*static_cast<T*>(source));
                }
                else {
                    error(type_name<T>() + " is not move constructible");
                }
            }
        }
        template<typename T>
        static bool componentComparator([[maybe_unused]] const void* lhs, [[maybe_unused]] const void* rhs) {
            if constexpr (detail::testOperatorEq<T>(nullptr)) {
                return *static_cast<const T*>(lhs) == *static_cast<const T*>(rhs);
            }
            else {
                error("Not implemented");
                return false;
            }
        }

        template <typename T>
        static ComponentInfo make() {
            static ComponentInfo result {
                sizeof(T),
                alignof(T),
                type_name<T>(),
                typeid(T).hash_code(),
                ComponentInfo::FunctionSet {
                        std::is_trivially_default_constructible<T>::value ? ComponentInfo::Constructor{} :
                                                                            ComponentInfo::componentConstructor<T>,
                        &copyComponent<T>,
                        &moveComponent<T>,
                        &componentMoveConstructor<T>,
                        std::is_trivially_destructible<T>::value ? ComponentInfo::Destructor{} : &componentDestructor<T>,
                        &componentComparator<T>
                }, {}
            };
            return result;
        }
    private:
        // may throw an exception
        static void error(const char* msg);

        static void error(const std::string& msg) {
            error(msg.c_str());
        }
    };
}