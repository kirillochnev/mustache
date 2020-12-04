#pragma once

#include <mustache/utils/default_settings.hpp>
#include <mustache/ecs/entity.hpp>

#include <tuple>

namespace mustache {

    class EntityManager;

    struct EntityBuilder {
        EntityManager* entity_manager;

        template<typename Component, typename... ARGS>
        MUSTACHE_INLINE auto assign(ARGS&&... args);

        MUSTACHE_INLINE Entity end();
    };

    template<typename _C, typename Args>
    struct ComponentArg {
        ComponentArg(Args&& args_):
                args {std::move(args_)} {

        }
        using Component = _C;
        static constexpr size_t num_args = std::tuple_size<Args>::value;
        Args args;
    };

    template<typename TupleType>
    struct ArgsPack {
        ArgsPack(EntityManager* manager, TupleType&& args):
                tuple {std::move(args)},
                entity_manager {manager} {

        }

        TupleType tuple;
        EntityManager* entity_manager;

        template<typename Component, typename... ARGS>
        auto assign(ARGS&&... args) {
            using ComponentArgType = ComponentArg<Component, decltype(std::forward_as_tuple(args...)) >;
            ComponentArgType arg {
                    std::forward_as_tuple(args...)
            };
            using NewTupleType = decltype(std::tuple_cat(tuple, std::make_tuple(std::move(arg))));
            return ArgsPack<NewTupleType> (entity_manager, std::tuple_cat(tuple, std::make_tuple(std::move(arg))));
        }

        MUSTACHE_INLINE Entity end();

    private:
        static constexpr size_t kTupleSize = std::tuple_size<TupleType>::value;

        template<size_t _I>
        void initComponents (Entity entity) const noexcept {
            if constexpr (_I < kTupleSize) {
                std::get<_I>(tuple).initComponent(entity);
                initComponents<_I + 1>(entity);
            }
        }
    };


}
