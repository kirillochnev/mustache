#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/job.hpp>
#include <gtest/gtest.h>
#include <map>

namespace {
    struct Position {
        uint32_t x = 0u;
        uint32_t y = 0u;
        uint32_t z = 0u;
    };
    struct Velocity {
        uint32_t value = 0u;
    };
    struct Orientation {
        uint32_t x = 0u;
        uint32_t y = 0u;
        uint32_t z = 0u;
    };

    struct Component0 {

    };

    struct Component1 {

    };

    struct Component2 {

    };

    struct Component3 {

    };

    struct UnusedComponent {

    };

    enum : uint32_t {
        kNumObjects = 1024,
        kNumIteration = 128,
    };

    struct {
        uint32_t count = 0;
        uint32_t cur_iteration = 0;
    } static static_data;

    struct CheckJob : public mustache::PerEntityJob<CheckJob> {
        void operator()(const Position& position, const Velocity& velocity, const Orientation& orientation) {
            --static_data.count;

            if (position.x != static_data.cur_iteration * velocity.value * orientation.x) {
                throw std::runtime_error("invalid position");
            }
            if (position.y != static_data.cur_iteration * velocity.value * orientation.y) {
                throw std::runtime_error("invalid position");
            }
            if (position.z != static_data.cur_iteration * velocity.value * orientation.z) {
                throw std::runtime_error("invalid position");
            }
        }
    };

    mustache::Dispatcher dispatcher;
}

TEST(Job, iterate_singlethread) {
    struct Job0 : public mustache::PerEntityJob<Job0> {
        void operator()(Position& position, const Velocity& velocity, const Orientation& orientation) {
            position.x += velocity.value * orientation.x;
            position.y += velocity.value * orientation.y;
            position.z += velocity.value * orientation.z;
            ++static_data.count;
        }
    };

    std::vector<mustache::Entity> created_entities;
    created_entities.reserve(kNumObjects);
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype<Position, Orientation, Velocity>();
    for (uint32_t i = 0; i < kNumObjects; ++i) {
        auto entity = entities.create(archetype);
        created_entities.push_back(entity);
        entities.getComponent<Velocity>(entity)->value = entity.id().toInt() % 128;
        *entities.getComponent<Orientation>(entity) = Orientation{1, 2, 3};
    }

    Job0 job_update;
    CheckJob job_check;
    for (uint32_t i = 0; i < kNumIteration; ++i) {
        ++static_data.cur_iteration;

        job_update.run(world, dispatcher);
        ASSERT_EQ(static_data.count, kNumObjects);

        job_check.run(world, dispatcher);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);
            position.x != static_data.cur_iteration * velocity.value * orientation.x;
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}


TEST(Job, iterate_singlethread_with_required_componen) {
    struct Job0 : public mustache::PerEntityJob<Job0> {
        void operator()(Position& position, const mustache::RequiredComponent<Velocity>& velocity, const Orientation& orientation) {
            position.x += velocity->value * orientation.x;
            position.y += velocity->value * orientation.y;
            position.z += velocity->value * orientation.z;
            ++static_data.count;
        }
    };

    std::vector<mustache::Entity> created_entities;
    created_entities.reserve(kNumObjects);
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype<Position, Orientation, Velocity>();
    for (uint32_t i = 0; i < kNumObjects; ++i) {
        auto entity = entities.create(archetype);
        created_entities.push_back(entity);
        entities.getComponent<Velocity>(entity)->value = entity.id().toInt() % 128;
        *entities.getComponent<Orientation>(entity) = Orientation{1, 2, 3};
    }

    Job0 job_update;
    CheckJob job_check;
    for (uint32_t i = 0; i < kNumIteration; ++i) {
        ++static_data.cur_iteration;

        job_update.run(world, dispatcher);
        ASSERT_EQ(static_data.count, kNumObjects);

        job_check.run(world, dispatcher);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);
            position.x != static_data.cur_iteration * velocity.value * orientation.x;
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}

TEST(Job, iterate_singlethread_with_optional_componen) {
    struct Job0 : public mustache::PerEntityJob<Job0> {
        void operator()(Position& position, const mustache::RequiredComponent<Velocity>& velocity,
                const Orientation& orientation, mustache::OptionalComponent<UnusedComponent> null) {
            if (null != nullptr) {
                throw std::runtime_error("invalid component");
            }
            position.x += velocity->value * orientation.x;
            position.y += velocity->value * orientation.y;
            position.z += velocity->value * orientation.z;
            ++static_data.count;
        }
    };
    std::vector<mustache::Entity> created_entities;
    created_entities.reserve(kNumObjects);
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype<Position, Orientation, Velocity>();
    for (uint32_t i = 0; i < kNumObjects; ++i) {
        auto entity = entities.create(archetype);
        created_entities.push_back(entity);
        entities.getComponent<Velocity>(entity)->value = entity.id().toInt() % 128;
        *entities.getComponent<Orientation>(entity) = Orientation{1, 2, 3};
    }

    Job0 job_update;
    CheckJob job_check;
    for (uint32_t i = 0; i < kNumIteration; ++i) {
        ++static_data.cur_iteration;

        job_update.run(world, dispatcher);
        ASSERT_EQ(static_data.count, kNumObjects);

        job_check.run(world, dispatcher);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);
            position.x != static_data.cur_iteration * velocity.value * orientation.x;
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}

TEST(Job, iterate_singlethread_4_archetypes_match_4_not) {
    static_assert(kNumObjects % 4 == 0);

    struct Job0 : public mustache::PerEntityJob<Job0> {
        void operator()(Position& position, const Velocity& velocity, const Orientation& orientation) {
            position.x += velocity.value * orientation.x;
            position.y += velocity.value * orientation.y;
            position.z += velocity.value * orientation.z;
            ++static_data.count;
        }
    };

    std::vector<mustache::Entity> created_entities;
    created_entities.reserve(kNumObjects);
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    std::array archetypes {
            &entities.getArchetype<Position, Orientation, Velocity, Component0>(),
            &entities.getArchetype<Position, Orientation, Velocity, Component1>(),
            &entities.getArchetype<Position, Orientation, Velocity, Component0, Component1>(),
            &entities.getArchetype<Position, Orientation, Velocity, Component2>(),
            &entities.getArchetype<Position, Orientation, Component2>(),
            &entities.getArchetype<Position, Velocity, Component2>(),
            &entities.getArchetype<Orientation, Velocity, Component3>(),
            &entities.getArchetype<Component0, Component1, Component2, Component3>()
    };

    for (uint32_t i = 0; i < kNumObjects / 4; ++i) {
        for (auto archetype : archetypes) {
            auto entity = entities.create(*archetype);
            if (entities.hasComponent<Position>(entity) && entities.hasComponent<Velocity>(entity) &&
                entities.hasComponent<Orientation>(entity)) {
                created_entities.push_back(entity);
            }
            if (entities.hasComponent<Velocity>(entity)) {
                entities.getComponent<Velocity>(entity)->value = entity.id().toInt() % 128;
            }
            if (entities.hasComponent<Orientation>(entity)) {
                *entities.getComponent<Orientation>(entity) = Orientation{1, 2, 3};
            }
        }
    }
    ASSERT_EQ(created_entities.size(), kNumObjects);
    Job0 job_update;
    CheckJob job_check;
    for (uint32_t i = 0; i < kNumIteration; ++i) {
        ++static_data.cur_iteration;

        job_update.run(world, dispatcher);
        ASSERT_EQ(static_data.count, kNumObjects);

        job_check.run(world, dispatcher);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);
            position.x != static_data.cur_iteration * velocity.value * orientation.x;
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}

TEST(Job, iterate_multithread_4_archetypes_match_4_not) {
    constexpr uint32_t kNumObjects2 = 1024 * 1024;
    constexpr uint32_t kNumIteration2 = 8;
    static_assert(kNumObjects2 % 4 == 0);
    constexpr uint32_t kTaskCount = 8;
    struct Job0 : public mustache::PerEntityJob<Job0> {
        struct alignas(128) NumUpdatedEntities {
            uint32_t value = 0;
        };
        std::array<NumUpdatedEntities, 8> counter;
        void operator()(Position& position, const Velocity& velocity, const Orientation& orientation,
                mustache::JobInvocationIndex job_invocation_index) {
            position.x += velocity.value * orientation.x;
            position.y += velocity.value * orientation.y;
            position.z += velocity.value * orientation.z;
            ++counter[job_invocation_index.task_index.toInt()].value;
        }
    };

    std::vector<mustache::Entity> created_entities;
    created_entities.reserve(kNumObjects2);
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    std::array archetypes {
            &entities.getArchetype<Position, Orientation, Velocity, Component0>(),
            &entities.getArchetype<Position, Orientation, Velocity, Component1>(),
            &entities.getArchetype<Position, Orientation, Velocity, Component0, Component1>(),
            &entities.getArchetype<Position, Orientation, Velocity, Component2>(),
            &entities.getArchetype<Position, Orientation, Component2>(),
            &entities.getArchetype<Position, Velocity, Component2>(),
            &entities.getArchetype<Orientation, Velocity, Component3>(),
            &entities.getArchetype<Component0, Component1, Component2, Component3>()
    };

    for (uint32_t i = 0; i < kNumObjects2 / 4; ++i) {
        for (auto archetype : archetypes) {
            auto entity = entities.create(*archetype);
            if (entities.hasComponent<Position>(entity) && entities.hasComponent<Velocity>(entity) &&
                entities.hasComponent<Orientation>(entity)) {
                created_entities.push_back(entity);
            }
            if (entities.hasComponent<Velocity>(entity)) {
                entities.getComponent<Velocity>(entity)->value = entity.id().toInt() % 128;
            }
            if (entities.hasComponent<Orientation>(entity)) {
                *entities.getComponent<Orientation>(entity) = Orientation{1, 2, 3};
            }
        }
    }
    ASSERT_EQ(created_entities.size(), kNumObjects2);
    Job0 job_update;
    CheckJob job_check;
    for (uint32_t i = 0; i < kNumIteration2; ++i) {
        ++static_data.cur_iteration;

        job_update.runParallel(world, kTaskCount, dispatcher);
        for(auto& count : job_update.counter) {
            static_data.count += count.value;
            count.value = 0u;
        }
        ASSERT_EQ(static_data.count, kNumObjects2);
        job_check.run(world, dispatcher);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);
            position.x != static_data.cur_iteration * velocity.value * orientation.x;
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}
