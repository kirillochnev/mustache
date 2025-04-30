#pragma once

#include <mustache/utils/dll_export.h>
#include <mustache/utils/type_info.hpp>
#include <mustache/utils/index_like.hpp>
#include <mustache/utils/container_map.hpp>
#include <mustache/utils/container_vector.hpp>

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

        template<typename C>
        inline constexpr bool hasBeforeRemove(decltype(&C::beforeRemove)) noexcept {
            return true;
        }

        template<typename C>
        inline constexpr bool hasBeforeRemove(...) noexcept {
            return false;
        }

        template<typename C>
        inline constexpr bool hasAfterAssign(decltype(&C::afterAssign)) noexcept {
            return true;
        }

        template<typename C>
        inline constexpr bool hasAfterAssign(...) noexcept {
            return false;
        }

        template<typename C>
        inline constexpr bool hasClone(decltype(&C::clone)) noexcept {
            return true;
        }
        template<typename C>
        inline constexpr bool hasClone(...) noexcept {
            return false;
        }

        template<typename C>
        inline constexpr bool hasAfterClone(decltype(&C::afterClone)) noexcept {
            return true;
        }
        template<typename C>
        inline constexpr bool hasAfterClone(...) noexcept {
            return false;
        }
    }


    class World;
    struct CloneEntityMap;
    struct Entity;

    template<typename _Sign>
    using Functor = std::function<_Sign>;

    template<typename T>
    struct CloneDest {
        const Entity& entity;
        T* value;
    };
    template<typename T>
    struct CloneSource {
        const Entity& entity;
        const T* value;
    };

    struct MUSTACHE_EXPORT ComponentInfo {
        // TODO: make functions works with N components in 1 call
        using Constructor = Functor<void (void*, const Entity&, World&) >;
        using CopyFunction = Functor<void (void*, const void*) >;
        using MoveFunction = Functor<void (void* dest, void* source) >;
        using Destructor = Functor<void (void*) >;
        using IsEqual = Functor<bool (const void*, const void*)>;
        using BeforeRemove = Functor<void(void*, const Entity&, World&)>;
        using AfterAssing = Functor<void(void*, const Entity&, World&)>;
        using CloneFunction = Functor<void(void*, const Entity&, const void*,
                                      const Entity&, World&, CloneEntityMap&)>;
        size_t size{0};
        size_t align{0};
        std::string name;
        size_t type_id_hash_code;
        struct FunctionSet {
            Constructor create;
            CopyFunction copy;
            MoveFunction move;
            MoveFunction move_constructor;
            MoveFunction move_constructor_and_destroy;
            Destructor destroy;
            IsEqual compare;
            BeforeRemove before_remove;
            AfterAssing after_assign;
            CloneFunction clone;
            CloneFunction after_clone;
        } functions;

        mustache::vector<std::byte> default_value; // this array will be used to init component in case of empty constructor

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
        static void afterComponentAssign(void *ptr, [[maybe_unused]] const Entity& entity, [[maybe_unused]] World& world) {
            if constexpr (detail::hasAfterAssign<T>(nullptr)) {
                const auto typed_ptr = static_cast<T*>(ptr);
                invoke(&T::afterAssign, typed_ptr, *typed_ptr, entity, world);
            }
        }

        template<typename T>
        static void beforeComponentRemove(void* ptr, [[maybe_unused]] const Entity& entity, [[maybe_unused]] World& world) {
            if constexpr (detail::hasBeforeRemove<T>(nullptr)) {
                const auto typed_ptr = static_cast<T*>(ptr);
                invoke(&T::beforeRemove, typed_ptr, *typed_ptr, entity, world);
            }
        }        
        template<typename T>
        static void componentDestructor(void* ptr) {
            static_cast<T*>(ptr)->~T();
        }

        template<typename T>
        static void clone(void* dest_ptr, const Entity& dest_entity, const void* source_ptr,
                          const Entity& source_entity, World& world, CloneEntityMap& map) {
            if constexpr (detail::hasClone<T>(nullptr)) {
                CloneSource<T> source {
                    source_entity,
                    static_cast<const T*>(source_ptr)
                };
                CloneDest<T> dest {
                    dest_entity,
                    static_cast<T*>(dest_ptr)
                };
                invoke(&T::clone, source, dest, world, map);
            } else {
                copyComponent<T>(dest_ptr, source_ptr);
            }
        }
        template<typename T>
        static void afterClone(void* dest_ptr, const Entity& dest_entity, const void* source_ptr,
                               const Entity& source_entity, World& world, CloneEntityMap& map) {
            if constexpr (detail::hasAfterClone<T>(nullptr)) {
                CloneSource<T> source {
                    source_entity,
                    static_cast<const T*>(source_ptr)
                };
                CloneDest<T> dest {
                    dest_entity,
                    static_cast<T*>(dest_ptr)
                };
                invoke(&T::afterClone, source, dest, world, map);
            }
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
        static void moveComponent(void* dest_void, void* source_void) {
            auto dest = static_cast<T*>(dest_void);
            auto source = static_cast<T*>(source_void);
            if (dest != source) {
                if constexpr (std::is_move_assignable_v<T>) {
                    *dest = std::move(*source);
                } else {
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        std::destroy(dest);
                    }
                    new(dest) T(std::move(source));
                }
            }
        }

        template<typename T>
        static void componentMoveConstructor([[maybe_unused]] void* dest_void, [[maybe_unused]] void* source_void) {
            auto dest = static_cast<T*>(dest_void);
            auto source = static_cast<T*>(source_void);
            if (source != dest) {
                if constexpr (std::is_move_constructible_v<T>) {
                    new(dest) T{std::move(*source)};
                } else {
                    if constexpr (std::is_default_constructible_v<T>) {
                        T* ptr = new(dest) T{};
                        *ptr = std::move(*source);
                    } else {
                        error(type_name<T>() + " is not move constructible");
                    }
                }
            }
        }

        template<typename T>
        static void componentMoveConstructorAndDestroy([[maybe_unused]] void* dest, [[maybe_unused]] void* source) {
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
            if constexpr (!std::is_trivially_destructible_v<T>) {
                std::destroy_at(reinterpret_cast<T*>(source));
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
                        &componentMoveConstructorAndDestroy<T>,
                        std::is_trivially_destructible<T>::value ? ComponentInfo::Destructor{} : &componentDestructor<T>,
                        &componentComparator<T>,
                        detail::hasBeforeRemove<T>(nullptr) ? &beforeComponentRemove<T> : ComponentInfo::BeforeRemove{},
                        detail::hasAfterAssign<T>(nullptr) ? &afterComponentAssign<T> : ComponentInfo::AfterAssing{},
                        &clone<T>,
                        detail::hasAfterClone<T>(nullptr) ? &afterClone<T> : ComponentInfo::CloneFunction{},

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
