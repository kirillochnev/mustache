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

TEST(EntityManager, lock_unlock) {
    constexpr uint32_t block_size = 3;
    struct Counters {
        int32_t constructor = 0;
        int32_t copy_constructor = 0;
        int32_t destructor = 0;
        int32_t move = 0;
        int32_t move_assign = 0;
        int32_t assign = 0;
    };
    static Counters counter;


    static int32_t count = 0;
    static bool error = false;
    static int32_t assign_event_calls = 0u;
    static int32_t remove_event_calls = 0u;
    struct CheckStateComponent {

        static void afterAssign() {
            ++assign_event_calls;
        }

        static void beforeRemove() {
            ++remove_event_calls;
        }

        CheckStateComponent() {
            ++counter.constructor;
            ++count;
        }
        CheckStateComponent(const CheckStateComponent&) {
            ++counter.copy_constructor;
            ++count;
        }
        CheckStateComponent(CheckStateComponent&&) {
            ++counter.move;
            ++count;
        }
        ~CheckStateComponent() {
            ++counter.destructor;
            --count;
            error = error || (count < 0);
        }
        CheckStateComponent& operator=(const CheckStateComponent&) {
            ++counter.assign;
            return *this;
        }
        CheckStateComponent& operator=(CheckStateComponent&&) {
            ++counter.move_assign;
            return *this;
        }
    };

    ASSERT_EQ(0, count);
    ASSERT_FALSE(error);
    {
        mustache::World world;
        auto& entities = world.entities();
        const auto check_count = [&entities](uint32_t expected) {
            uint32_t result = 0u;
            entities.forEach([&result, expected](const CheckStateComponent&) {
                ++result;
                if (result > expected) {
                    throw std::runtime_error("Out of range");
                }
            });
            if (expected != result) {
                throw std::runtime_error("Invalid count: " + std::to_string(expected) + " vs " + std::to_string(result));
            }
            if (expected != assign_event_calls - remove_event_calls) {

                throw std::runtime_error("Invalid events count | assign_event_calls: " + 
                    std::to_string(assign_event_calls) + " remove_event_calls: " + std::to_string(remove_event_calls));
            }
        };


        int32_t expected_count = 0;
        const auto on_assign = [&expected_count] {
            ++expected_count;
        };
        const auto on_remove = [&expected_count] {
            --expected_count;
            if (expected_count < 0) {
                throw std::runtime_error("WTF?!");
            }
        };
        for (uint32_t i = 0; i < block_size; ++i) {
            auto entity = entities.create<CheckStateComponent, Component0, Component1 >();
            on_assign();
            entities.assign<Component2>(entity);
            entities.assign<Component3>(entity);

            check_count(expected_count);
            ASSERT_EQ(expected_count, count);
            ASSERT_FALSE(error);
        }
        ASSERT_FALSE(error);
        auto before_lock_count = expected_count;
        entities.lock();
        for (uint32_t i = 0; i < block_size; ++i) {
            auto entity = entities.create<Component0, Component1 >(); 
            on_assign();
            entities.assign<CheckStateComponent >(entity);
            entities.assign<Component2 >(entity);
            entities.assign<Component3>(entity);

            check_count(before_lock_count);
            ASSERT_EQ(expected_count, count);
            ASSERT_FALSE(error);
        }
        ASSERT_FALSE(error);
        for (uint32_t i = 0; i < block_size; ++i) {
            auto entity = entities.create<CheckStateComponent, Component0, Component1 >();
            on_assign();
            entities.assign<Component3>(entity);
            entities.removeComponent<CheckStateComponent >(entity);
            on_remove();
            entities.assign<Component2>(entity);

            check_count(before_lock_count);
            ASSERT_EQ(expected_count, count);
            ASSERT_FALSE(error);
        }

        ASSERT_FALSE(error);
        entities.unlock();

        ASSERT_FALSE(error);
        check_count(expected_count);
        ASSERT_EQ(expected_count, count);

        entities.forEach([&](mustache::Entity entity, const CheckStateComponent* ptr) {
            if (ptr != nullptr) {
                on_remove();
                entities.removeComponent<CheckStateComponent >(entity);
            }
            else {
                on_assign();
                entities.assign<CheckStateComponent>(entity);
            }
        });

        check_count(expected_count);
        ASSERT_EQ(expected_count, count);

        const uint32_t active_count = count;
        uint32_t remove_count = 0;
        entities.forEach([&](mustache::Entity entity, const CheckStateComponent&) {
            entities.removeComponent<CheckStateComponent >(entity);
            on_remove();
            ++remove_count;
        });

        ASSERT_EQ(active_count, remove_count);
        ASSERT_EQ(0, count);
        check_count(0);
    }
    ASSERT_EQ(0, count);
    ASSERT_FALSE(error);
}
