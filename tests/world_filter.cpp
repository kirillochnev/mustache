#include <gtest/gtest.h>
#include <mustache/ecs/world_filter.hpp>
#include <mustache/ecs/job.hpp>
#include <mustache/utils/benchmark.hpp>
#include <mustache/utils/dispatch.hpp>

namespace {
    template<size_t, size_t _Size = 256>
    struct Component {
        std::array<std::byte, _Size> data;
    };

    struct WorldKeeper {
        mustache::World world;
        WorldKeeper():
            world(mustache::WorldId::make(0)) {
            auto& entities = world.entities();
            for (uint32_t i = 0; i < 1024 * 1024; ++i) {
                (void) entities.create<Component<0> >();
                (void) entities.create<Component<0>, Component<1> >();
                (void) entities.create<Component<0>, Component<2> >();
                (void) entities.create<Component<0>, Component<3> >();
                (void) entities.create<Component<0>, Component<4> >();
            }
        }
    };

    mustache::World& getWorld() {
        static WorldKeeper world_keeper;
        return world_keeper.world;
    }
}

namespace {
    struct TaskView {
        mustache::ComponentIdMask required_;
        mustache::ComponentIdMask optional_;
        int* begin();
        int* end();
    };
    struct World_ {
        template<size_t I>
        using Components = std::array<mustache::ComponentId, I>;

        TaskView view(const mustache::ComponentIdMask& required, const mustache::ComponentIdMask& optional = {}) {
            return TaskView{required, optional};
        }
    };
    struct Job {
        bool is_view_init_ = false;
        TaskView view_;
        mustache::ComponentIdMask mask_;


        void operator()() {

        }
        void initView(World_& world) {
            view_ = world.view(mask_);
            is_view_init_ = true;
        }
        void run(World_& world) {
            if (!is_view_init_) {
                initView(world);
            }
            for (auto it : view_) {
                (*this)();
            }
        }
    };
}
TEST(WorldFilter, POC) {

}

TEST(WorldFilter, component_version) {

    struct Job : mustache::PerEntityJob<Job> {
        void operator() (Component<0>& component) {
            (void ) component;
//            component.data[0] = std::byte{0};
        }
    };
    auto& world = getWorld();
    auto& entities = world.entities();
    mustache::Dispatcher dispatcher;
    auto entity = entities.create<Component<0> >();
    ASSERT_EQ(world.version(), mustache::WorldVersion::make(0));
    ASSERT_EQ(world.version(), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity));
    world.update();
    ASSERT_EQ(world.version(), mustache::WorldVersion::make(1));
    ASSERT_EQ(mustache::WorldVersion::make(0), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity));
    world.update();
    entities.getComponent<Component<0> >(entity)->data.data()[0] = std::byte{0};
    ASSERT_EQ(world.version(), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity));
    world.update();
    ASSERT_EQ(world.version(), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity).next());
    entities.getComponent<const Component<0> >(entity);
    ASSERT_EQ(world.version(), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity).next());
    Job job;
    job.run(world, dispatcher);
//    ASSERT_EQ(world.version(), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity).next().next());
}
