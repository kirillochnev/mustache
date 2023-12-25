#pragma once

#include <mustache/utils/default_settings.hpp>

#include <mustache/ecs/entity.hpp>
#include <mustache/ecs/component_mask.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <tuple>

namespace mustache {

    class EntityManager;

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
    class EntityBuilder;

    template<>
    class EntityBuilder<void> {
    public:
        template<typename Component, typename... ARGS>
        MUSTACHE_INLINE auto assign(ARGS&&... args);

        template<typename T>
        MUSTACHE_INLINE auto& remove() noexcept {
            to_destroy_.add(ComponentFactory::instance().registerComponent<T>());
            return *this;
        }

        MUSTACHE_INLINE Entity end();

    private:
        friend EntityManager;
        explicit EntityBuilder(EntityManager* manager, Entity e):
                entity_{e},
                to_destroy_{},
                entity_manager_{manager} {

        }
        Entity entity_;
        ComponentIdMask to_destroy_;
        EntityManager* entity_manager_;
    };

    template<typename TupleType>
    class EntityBuilder {
    public:
        EntityBuilder(const ComponentIdMask& to_destroy, EntityManager* manager,  Entity e, TupleType&& args):
                args_ {std::move(args)},
                entity_{e},
                to_destroy_{to_destroy},
                entity_manager_ {manager} {

        }
        template<typename T>
        MUSTACHE_INLINE auto& remove() noexcept {
            to_destroy_.add(ComponentFactory::instance().registerComponent<T>());
            return *this;
        }

        template<typename Component, typename... ARGS>
        auto assign(ARGS&&... args) {
            using ComponentArgType = ComponentArg<Component, decltype(std::forward_as_tuple(args...)) >;
            ComponentArgType arg {
                    std::forward_as_tuple(args...)
            };
            using NewTupleType = decltype(std::tuple_cat(args_, std::make_tuple(std::move(arg))));
            return EntityBuilder<NewTupleType> (to_destroy_, entity_manager_, entity_,
                                                std::tuple_cat(args_, std::make_tuple(std::move(arg))));
        }

        MUSTACHE_INLINE Entity end();

        TupleType& getArgs() noexcept {
            return args_;
        }
    private:
        TupleType args_;
        Entity entity_;
        ComponentIdMask to_destroy_;
        EntityManager* entity_manager_;


        static constexpr size_t kTupleSize = std::tuple_size<TupleType>::value;

        template<size_t _I>
        void initComponents (Entity entity) const noexcept {
            if constexpr (_I < kTupleSize) {
                std::get<_I>(args_).initComponent(entity);
                initComponents<_I + 1>(entity);
            }
        }
    };
}
