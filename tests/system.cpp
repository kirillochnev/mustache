#include <gtest/gtest.h>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/system.hpp>
#include <mustache/ecs/job.hpp>

namespace {
    std::vector<size_t> systems_updated;
}

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
//        std::cout << _I  << ", ";
        systems_updated.push_back(_I);
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

    systems_updated.clear();

    World world;
    // depends on system<0>, order: 2, system<1024> does not exists
    world.systems().addSystem<SystemToUpdate<0> >(systemNames<1, 1024>());
    world.systems().addSystem<SystemToUpdate<1> >(systemNames<>()); // no dependencies, order: 1
    // systems<2-6> depend on system<0>, order so they will be sorted by priority
    world.systems().addSystem<SystemToUpdate<2> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<3> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<4> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<5> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<6> >(systemNames<0>());
    world.systems().addSystem<SystemToUpdate<7> >(systemNames<0, 3>());

    // systems<8-135> depend on system<2>, order so they will be sorted by priority
    addSystem<2, 8>(world.systems(), std::make_index_sequence<128>());

    world.systems().init();
    world.update();

    std::vector<size_t > expected_order {
        1, 0, 6, 5, 4, 3, 7, 2, 135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 119,
        118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96,
        95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68,
        67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40,
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
        11, 10, 9, 8,
    };
    ASSERT_EQ(expected_order, systems_updated);
}
