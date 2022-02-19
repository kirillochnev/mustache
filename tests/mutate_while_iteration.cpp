#include <mustache/ecs/ecs.hpp>

#include <mustache/utils/benchmark.hpp>

#include <gtest/gtest.h>

namespace {
    constexpr uint32_t N = 1024 * 1024;

    struct Component0 {
        uint32_t value = 1u;
    };
    struct Component1 {
        float value = 1.23f;
    };
    struct Component2 {
        std::string value = "value: 123567";
    };
    struct Component3 {
        std::shared_ptr<std::string > value = std::make_shared<std::string >("value: 7777");
    };
    struct Component4 {
        const char* value = "Hello, World";
    };

    void update(mustache::World& world) {
        world.entities().forEach([&entities = world.entities()](mustache::Entity e) {
            if (e.id().toInt() % 2u) {
                entities.removeComponent<Component0>(e);
            }
            if (e.id().toInt() % 3u) {
                entities.removeComponent<Component1>(e);
            }
            if (e.id().toInt() % 5u) {
                entities.removeComponent<Component2>(e);
            }
            if (e.id().toInt() % 7u) {
                entities.assign<Component3>(e);
            }
            if (e.id().toInt() % 11u) {
                entities.assign<Component4>(e);
            }
            if (e.id().toInt() % 13u) {
                entities.destroy(e);
            }
            if((e.id().toInt() % 17u) == 0u) {
                entities.destroyNow(e);
            }
        }, mustache::JobRunMode::kParallel);
    }
    void create(mustache::World& world) {
        for (uint32_t i = 0; i < N; ++i) {
            auto e = world.entities().begin()
                    .assign<Component0>()
                    .assign<Component1>()
                    .assign<Component2>()
                    .end();
        }
    }
    void check(mustache::World& world) {
        world.entities().forEach([](mustache::World& w, mustache::Entity e, const Component0* c0,
                                    const Component1* c1, const Component2* c2, const Component3* c3, const Component4* c4){
            if (e.id().toInt() % 2u) {
                ASSERT_EQ(c0, nullptr);
            } else {
                ASSERT_NE(c0, nullptr);
            }
            if (e.id().toInt() % 3u) {
                ASSERT_EQ(c1, nullptr);
            } else {
                ASSERT_NE(c1, nullptr);
            }
            if (e.id().toInt() % 5u) {
                ASSERT_EQ(c2, nullptr);
            } else {
                ASSERT_NE(c2, nullptr);
            }
            if (e.id().toInt() % 7u) {
                ASSERT_NE(c3, nullptr);
            } else {
                ASSERT_EQ(c3, nullptr);
            }
            if (e.id().toInt() % 11u) {
                ASSERT_NE(c4, nullptr);
            } else {
                ASSERT_EQ(c4, nullptr);
            }

            if (e.id().toInt() % 13u) {
                ASSERT_TRUE(w.entities().isMarkedForDestroy(e));
            }

            ASSERT_NE(e.id().toInt() % 17u, 0u);
        }, mustache::JobRunMode::kParallel);
    }
}
//TEST(EntityManager, update_while_iteration) {
void EntityManager_update_while_iteration() {
    mustache::Benchmark bench;
    mustache::World world;

    for (uint32_t i = 0; i < 10; ++i) {
        world.entities().clear();
        create(world);
        bench.add([&world] { update(world); });
        check(world);
    }
    bench.show();
}
