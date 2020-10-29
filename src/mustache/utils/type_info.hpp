#pragma once

#include <string>
#include <functional>

#ifdef _MSC_BUILD
#define MUSTACHE_FUNCTION_SIGNATURE __FUNCSIG__
#else
#define MUSTACHE_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif
namespace mustache {
    namespace detail {
        std::string make_type_name_from_func_name(const char* func_name) noexcept;
    }

    template <typename T>
    std::string type_name() noexcept {
        static std::string result = detail::make_type_name_from_func_name(MUSTACHE_FUNCTION_SIGNATURE);
        return result;
    }
    template<typename _Sign>
    using Functor = _Sign* ;//std::function<_Sign>;

    struct TypeInfo {
        using Constructor = Functor<void (void*) >;
        using CopyFunction = Functor<void (void*, const void*) >;
        using MoveFunction = Functor<void (void*, void*) >;
        using Destructor = Functor<void (void*) >;

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

        } functions;
    };

    template <typename T>
    const TypeInfo& makeTypeInfo() {
        static TypeInfo result {
                sizeof(T),
                alignof(T),
                type_name<T>(),
                typeid(T).hash_code(),
                TypeInfo::FunctionSet {
                        std::is_trivially_default_constructible<T>::value ? TypeInfo::Constructor{} : [](void *ptr) {
                            new(ptr) T;
                        },
                        [](void* dest, const void* source) {
                            if constexpr (std::is_copy_constructible_v<T>) {
                                new(dest) T{*static_cast<const T *>(source)};
                            }else {
                                (void)dest;
                                (void)source;
                                throw std::runtime_error(type_name<T>() + " is not copy constructible");
                            }
                        },
                        [](void* dest, void* source) {
                            *static_cast<T*>(dest) = std::move(*static_cast<T *>(source));
                        },
                        [](void* dest, void* source) {
                            if constexpr (std::is_move_constructible_v<T>) {
                                new(dest) T{std::move(*static_cast<T *>(source))};
                            } else {
                                if constexpr (std::is_default_constructible_v<T>) {
                                    T* ptr = new(dest) T{};
                                    *std::launder(ptr) = std::move(*static_cast<T *>(source));
                                } else {
                                    (void)dest;
                                    (void) source;
                                    throw std::runtime_error(type_name<T>() + " is not move constructible");
                                }
                            }
                        },
                        std::is_trivially_destructible<T>::value ? TypeInfo::Destructor{} : [](void *ptr) {
                            static_cast<T *>(ptr)->~T();
                        },
                }
        };
        return result;
    }
}