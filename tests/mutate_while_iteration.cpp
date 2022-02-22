#include <mustache/ecs/ecs.hpp>

#include <gtest/gtest.h>

namespace {
    constexpr uint32_t N = 1000000;//1024 * 1024;

    struct Component0 {
        uint32_t value = 1u;
    };
    struct Component1 {
        float value = 1.23f;
    };
    struct Component2 {
        static std::string str(uint32_t i = 0) noexcept {
            static const std::string res = "value: ";
            return res + std::to_string(i);
        }
        std::string value = str();
    };
    struct Component3 {
        static std::string str(uint32_t i = 0) noexcept {
            static const std::string res = "Super-puper string: ";
            return res + std::to_string(i);
        }
        std::shared_ptr<std::string > value = std::make_shared<std::string >(str());
    };
    struct Component4 {
        static const std::string str(uint32_t i = 0) noexcept {
            static const std::string res = "This is a very long string, very, very, very, very, very, very"
                                           ", very, very, very, very, very, very, very, very, very, very, very, very"
                                           ", very, very, very, very, very, very, very, very, very, very, very, very"
                                           ", very, very, very, very, very, very, very, very, very, very, very, very"
                                           ", very, very, very, very, very, very, very, very, very, very, very, very"
                                           ", very, very, very, very, very, very, very, very, very, very, very, very"
                                           ", very, very, very, very, very, very, very, very, very, very, very, very"
                                           ", very, very, very, very, very, very, very, very, very, very, very, very"
                                           ", very, very, very, very, very, very, very, very, very, very, very, very";
            return res + std::to_string(i);
        }
        std::string value = "Hello, World";
    };

    bool test(uint32_t id, uint32_t value) {
        return id % value == 0u;
    }

    void updateEntity(mustache::World& world, mustache::Entity entity) {
        auto& entities = world.entities();
        const auto id = entity.id().toInt();
        if (test(id, 2u)) {
            entities.removeComponent<Component0>(entity);
        }
        if (test(id, 3u)) {
            entities.begin(entity).remove<Component1>().end();
        }
        if (test(id, 5u)) {
            entities.removeComponent<Component2>(entity);
        }
        if (test(id, 7u)) {
            entities.assign<Component3>(entity, std::make_shared<std::string >(Component3::str(id)));
        }
        if (test(id, 11u)) {
            entities.begin(entity).assign<Component4>(Component4::str(id)).end();
        }
        if (test(id, 13u)) {
            entities.destroy(entity);
        }
        if(!test(id, 17u)) {
            entities.destroyNow(entity);
        }
        if (test(id, 19u)) {
            auto e = entities.begin().assign<Component0>().assign<Component1>().assign<Component4>().end();
            if (test(e.id().toInt(), 2u)) {
                entities.assign<Component3>(e);
            }
        }
    }
    void update(mustache::World& world, const std::vector<mustache::Entity>& arr) {
        for (auto entity : arr) {
            updateEntity(world, entity);
        }
    }

    void update(mustache::World& world) {
        world.entities().forEach(updateEntity, mustache::JobRunMode::kCurrentThread);
    }
    std::vector<mustache::Entity> create(mustache::World& world) {
        std::vector<mustache::Entity> res(N);
        for (uint32_t i = 0; i < N; ++i) {
            res[i] = world.entities().begin()
                .assign<std::array<std::byte, 128u>>()
                .assign<Component0>(i)
                .assign<Component1>( static_cast<float>(i) * 10.0f)
                .assign<Component2>(Component2::str(i))
            .end();
        }
        return res;
    }
    void check(mustache::World& world) {
        world.entities().forEach([](mustache::World& w, mustache::Entity e, const Component0* c0,
                                    const Component1* c1, const Component2* c2, const Component3* c3, const Component4* c4){
            const auto id = e.id().toInt();
            if (id >= N) {
                ASSERT_NE(c0, nullptr);
                ASSERT_NE(c1, nullptr);
                ASSERT_EQ(c2, nullptr);
                if (test(id, 2)) {
                    ASSERT_NE(c3, nullptr);
                } else {
                    ASSERT_EQ(c3, nullptr);
                }
                ASSERT_NE(c4, nullptr);
                return;
            }
            if (test(id, 2u)) {
                ASSERT_EQ(c0, nullptr);
            } else {
                ASSERT_NE(c0, nullptr);
                ASSERT_EQ(c0->value, id);
            }

            if (test(id, 3u)) {
                ASSERT_EQ(c1, nullptr);
            } else {
                ASSERT_NE(c1, nullptr);
                ASSERT_EQ(c1->value, static_cast<float>(id) * 10.0f);
            }
            if (test(id, 5u)) {
                ASSERT_EQ(c2, nullptr);
            } else {
                ASSERT_NE(c2, nullptr);
                ASSERT_EQ(c2->value, Component2::str(id));
            }
            if (test(id, 7u)) {
                ASSERT_NE(c3, nullptr);
                ASSERT_EQ(*c3->value, Component3::str(id));
            } else {
                ASSERT_EQ(c3, nullptr);
            }
            if (test(id, 11u)) {
                ASSERT_NE(c4, nullptr);
                ASSERT_EQ(c4->value, Component4::str(id));
            } else {
                ASSERT_EQ(c4, nullptr);
            }

            if (test(id, 13u)) {
                ASSERT_TRUE(w.entities().isMarkedForDestroy(e));
            }

            ASSERT_NE(test(id, 17u), 0u);
        }, mustache::JobRunMode::kParallel);
    }
}

TEST(EntityManager, update_while_iteration) {
    mustache::World world;

    const auto arr = create(world);
    update(world);
    check(world);
}
