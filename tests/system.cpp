#include <gtest/gtest.h>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/system.hpp>
#include <mustache/ecs/job.hpp>

template<size_t _I>
struct SystemToUpdate : public mustache::System<SystemToUpdate<_I> > {

    SystemToUpdate(const std::set<std::string >& update_after = {}):
        update_after_{update_after} {

    }
    void onConfigure(mustache::World&, mustache::SystemConfig& config) override {
        config.update_after = update_after_;
        config.priority = _I;
    }
    void onUpdate(mustache::World&) override {
        std::cout << _I  << " updated after: ";
        for (const auto& str : update_after_) {
            std::cout<< str << " ";
        }
        std::cout << std::endl;
    }

    std::set<std::string > update_after_;
};
namespace {
    template<size_t... _I>
    std::set<std::string> systemNames() {
        return std::set<std::string> {
            SystemToUpdate<_I>::systemName()...
        };
    }

    template<size_t _DependOn, size_t _First, size_t... _I>
    void addSystem(mustache::SystemManager& system_manager, std::index_sequence<_I...>&&) {
        std::array array {
            system_manager.addSystem<SystemToUpdate<_I + _First> >(std::set<std::string> {
                    SystemToUpdate<_DependOn>::systemName()
            })...
        };
        (void )array;
    }
}


TEST(System, system_order) {
    using namespace mustache;

    World world;
    world.systems().addSystem<SystemToUpdate<0> >(systemNames<1, 1024>());
    world.systems().addSystem<SystemToUpdate<1> >(systemNames<>());
    world.systems().addSystem<SystemToUpdate<2> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<3> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<4> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<5> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<6> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<7> >(systemNames<0, 3>());
    addSystem<2, 8>(world.systems(), std::make_index_sequence<128>());
    world.systems().init();
    world.update();
}

TEST(System, test1) {
    struct Position {
        float value;
    };
    struct Velocity {
        float value;
    };
    struct Acceleration {
        float value;
    };

    using namespace mustache;
    class System1 : public System<System1> {
    protected:
        void onUpdate(World& world) override {
            job_.run(world);
        }

    private:
        struct UpdatePosJob : public PerEntityJob<UpdatePosJob> {
            float dt;
            void operator()(Position& position, Velocity& velocity, const Acceleration& acceleration) {
                const auto delta_velocity = acceleration.value * dt;
                position.value += velocity.value * dt + delta_velocity * dt * 0.5;
                velocity.value += delta_velocity;
            }
        };
        UpdatePosJob job_;
    };

    World world;
    world.systems().addSystem<System1>();
    world.systems().init();
    world.update();
}
