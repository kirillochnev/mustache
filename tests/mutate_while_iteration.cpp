#include <mustache/ecs/ecs.hpp>

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

    void updateEntity(mustache::World& world, mustache::Entity entity) {
        auto& entities = world.entities();
        const auto id = entity.id().toInt();
        if (id % 2u) {
            entities.removeComponent<Component0>(entity);
        }
        if (id % 3u) {
            entities.removeComponent<Component1>(entity);
        }
        if (id % 5u) {
            entities.removeComponent<Component2>(entity);
        }
        if (id % 7u) {
            entities.assign<Component3>(entity, std::make_shared<std::string >(Component3::str(id)));
        }
        if (id % 11u) {
            entities.assign<Component4>(entity, Component4::str(id));
        }
        if (id % 13u) {
            entities.destroy(entity);
        }
        if((id % 17u) == 0u) {
            entities.destroyNow(entity);
        }
    }
    void update(mustache::World& world, const std::vector<mustache::Entity>& arr) {
        for (auto entity : arr) {
            updateEntity(world, entity);
        }
    }

    void update(mustache::World& world) {
        world.entities().forEach(updateEntity, mustache::JobRunMode::kParallel);
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
            if (id % 2u) {
                ASSERT_EQ(c0, nullptr);
            } else {
                ASSERT_NE(c0, nullptr);
                ASSERT_EQ(c0->value, id);
            }

            if (id % 3u) {
                ASSERT_EQ(c1, nullptr);
            } else {
                ASSERT_NE(c1, nullptr);
                ASSERT_EQ(c1->value, static_cast<float>(id) * 10.0f);
            }
            if (id % 5u) {
                ASSERT_EQ(c2, nullptr);
            } else {
                ASSERT_NE(c2, nullptr);
                ASSERT_EQ(c2->value, Component2::str(id));
            }
            if (id % 7u) {
                ASSERT_NE(c3, nullptr);
                ASSERT_EQ(*c3->value, Component3::str(id));
            } else {
                ASSERT_EQ(c3, nullptr);
            }
            if (id % 11u) {
                ASSERT_NE(c4, nullptr);
                ASSERT_EQ(c4->value, Component4::str(id));
            } else {
                ASSERT_EQ(c4, nullptr);
            }

            if (id % 13u) {
                ASSERT_TRUE(w.entities().isMarkedForDestroy(e));
            }

            ASSERT_NE(id % 17u, 0u);
        }, mustache::JobRunMode::kParallel);
    }
}
TEST(EntityManager, update_while_iteration) {
    mustache::World world;

    const auto arr = create(world);
    update(world);
    check(world);
}
