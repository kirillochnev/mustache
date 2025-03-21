#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/job.hpp>
#include <mustache/ecs/non_template_job.hpp>
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
        void reset() {
            cur_iteration = 0;
            count = 0;
        }
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
}

TEST(Job, iterate_empty) {
    mustache::World world;
    for (uint32_t i = 0; i < kNumObjects; ++i) {
        (void ) world.entities().create();
    }
    uint32_t count = 0;
    world.entities().forEach([&count] {
        ++count;
    }, mustache::JobRunMode::kCurrentThread);
    ASSERT_EQ(count, kNumObjects);

    for (uint32_t i = 0; i < kNumObjects; ++i) {
        (void ) world.entities().create<Position>();
        (void ) world.entities().create<Velocity>();
    }

    count = 0u;
    world.entities().forEach([&count] {
        ++count;
    }, mustache::JobRunMode::kCurrentThread);
    ASSERT_EQ(count, 3u * kNumObjects);

    count = 0u;
    world.entities().forEach([&count](mustache::Entity) {
        ++count;
    }, mustache::JobRunMode::kCurrentThread);
    ASSERT_EQ(count, 3u * kNumObjects);
}

TEST(Job, iterate_singlethread) {
    static_data.reset();

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

        job_update.run(world);
        ASSERT_EQ(static_data.count, kNumObjects);

        job_check.run(world);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);

            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}

TEST(Job, iterate_singlethread_with_required_componen) {
    static_data.reset();

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

        job_update.run(world);
        ASSERT_EQ(static_data.count, kNumObjects);

        job_check.run(world);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}

TEST(Job, iterate_singlethread_with_optional_componen) {
    static_data.reset();

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

        job_update.run(world);
        ASSERT_EQ(static_data.count, kNumObjects);

        job_check.run(world);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);

            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}

TEST(Job, iterate_singlethread_4_archetypes_match_4_not) {
    static_data.reset();

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

        job_update.run(world);
        ASSERT_EQ(static_data.count, kNumObjects);

        job_check.run(world);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);

            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}

TEST(Job, iterate_multithread_4_archetypes_match_4_not) {
    static_data.reset();

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

        job_update.run(world);
        for(auto& count : job_update.counter) {
            static_data.count += count.value;
            count.value = 0u;
        }
        ASSERT_EQ(static_data.count, kNumObjects2);
        job_check.run(world);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : created_entities) {
            ASSERT_EQ(entity.id().toInt() % 128, entities.getComponent<Velocity>(entity)->value);
            const auto& position = *entities.getComponent<Position>(entity);
            const auto& velocity = *entities.getComponent<Velocity>(entity);
            const auto& orientation = *entities.getComponent<Orientation>(entity);

            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
}

TEST(Job, iterate_and_check_value) {
    static_data.reset();

    struct SharedPositionComponent {
        std::shared_ptr<Position> value {new Position{0,0,0}};
    };

    struct PosInfo {
        SharedPositionComponent ptr;
        mustache::Entity entity;
    };
    std::vector<PosInfo > pos_array;
    std::vector<mustache::Entity> created_entities;
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    struct UpdateJob : public mustache::PerEntityJob<UpdateJob> {
        void operator()(SharedPositionComponent position, const Velocity& velocity, const Orientation& orientation) {
            position.value->x += velocity.value * orientation.x;
            position.value->y += velocity.value * orientation.y;
            position.value->z += velocity.value * orientation.z;
            ++static_data.count;
        }
    };
    std::array archetypes {
            &entities.getArchetype<SharedPositionComponent, Orientation, Velocity, Component0>(),
            &entities.getArchetype<SharedPositionComponent, Velocity, Component1>(),
            &entities.getArchetype<SharedPositionComponent, Orientation >(),
            &entities.getArchetype<SharedPositionComponent, Orientation, Velocity, Component2>(),
    };

    for (uint32_t i = 0; i < kNumObjects / 2; ++i) {
        for (auto archetype : archetypes) {
            auto entity = entities.create(*archetype);
            if (entities.hasComponent<SharedPositionComponent>(entity) && entities.hasComponent<Velocity>(entity) &&
                entities.hasComponent<Orientation>(entity)) {
                created_entities.push_back(entity);
                PosInfo info;
                info.entity = entity;
                info.ptr = *entities.getComponent<SharedPositionComponent>(entity);
                pos_array.push_back(info);
            }
            if (entities.hasComponent<Velocity>(entity)) {
                entities.getComponent<Velocity>(entity)->value = entity.id().toInt() % 128;
            }
            if (entities.hasComponent<Orientation>(entity)) {
                *entities.getComponent<Orientation>(entity) = Orientation{1, 2, 3};
            }
        }
    }
    ASSERT_EQ(pos_array.size(), kNumObjects);

    UpdateJob update_job;
    for (uint32_t i = 0; i < kNumIteration; ++i) {
        ++static_data.cur_iteration;

        update_job.run(world);

        ASSERT_EQ(static_data.count, kNumObjects);

        for (const auto& info : pos_array) {
            ASSERT_TRUE(entities.hasComponent<SharedPositionComponent>(info.entity));
            ASSERT_TRUE(entities.hasComponent<Velocity>(info.entity));
            ASSERT_TRUE(entities.hasComponent<Orientation>(info.entity));
            const auto& pos_ptr = entities.getComponent<SharedPositionComponent>(info.entity)->value;
            ASSERT_EQ(pos_ptr, info.ptr.value);
            const Position& position = *pos_ptr;
            const Velocity& velocity = *entities.getComponent<Velocity>(info.entity);
            const Orientation& orientation = *entities.getComponent<Orientation>(info.entity);
            ASSERT_EQ(position.x, static_data.cur_iteration * velocity.value * orientation.x);
            ASSERT_EQ(position.y, static_data.cur_iteration * velocity.value * orientation.y);
            ASSERT_EQ(position.z, static_data.cur_iteration * velocity.value * orientation.z);
            --static_data.count;
        }

        ASSERT_EQ(static_data.count, 0);
    }
}

TEST(Job, compilation) {
    mustache::World world;
    struct TestComponent0 {};
    struct TestComponent1 {};
    struct TestTask : public mustache::PerEntityJob<TestTask> {
        void operator() (mustache::Entity, const TestComponent0&, TestComponent1&, mustache::JobInvocationIndex) {
        }
    };
    TestTask task;
    task.run(world);

    using Info = mustache::JobInfo<TestTask>::FunctionInfo;
    static_assert(Info::any_components_count == 2);
    static_assert(Info::components_count == 2);
    static_assert(Info::shared_components_count == 0);
    static_assert(Info::args_count == 4);
    static_assert(std::is_same<Info::UniqueComponentType<0>::type, const TestComponent0&>::value);
    static_assert(std::is_same<Info::UniqueComponentType<1>::type, TestComponent1&>::value);
}

TEST(Job, UpdateSingleChunk) { // Should fail
    struct Component0 {
        uint32_t value = 0;
    };

    struct UpdateJob : public mustache::PerEntityJob<UpdateJob> {
        void operator()(Component0& component_0, mustache::JobInvocationIndex index, mustache::Entity e) {
            if (show_up) {
                std::cout << "Task: " << index.task_index.toInt() << "  |  Entity: " << e.id().toInt() << std::endl;
            }
            ++component_0.value;
            ++count;
        }

        mustache::ComponentIdMask checkMask() const noexcept override {
            return mustache::ComponentFactory::instance().makeMask<Component0>();
        }
        uint32_t count = 0;
        bool show_up = false;
    };
    std::vector<mustache::Entity> entities_created;
    mustache::World world;
    world.dispatcher().setSingleThreadMode(true);
    auto& entities = world.entities();

    uint32_t expected_count = 0u;
    const auto& update = [&entities_created, &entities, &expected_count](uint32_t first, uint32_t count) {
        for (uint32_t i = first; i < first + count; ++i) {
            entities.getComponent<Component0>(entities_created[i])->value = 0;
        }
        expected_count += count;
    };

    entities.addChunkSizeFunction<Component0>(16, 16);

    for (uint32_t i = 0; i < kNumObjects * 10; ++i) {
        entities_created.push_back(entities.create<Component0>());
    }

    UpdateJob update_job;
    update_job.run(world, mustache::JobRunMode::kParallel);
    for (auto e : entities_created) {
        auto ptr = entities.getComponent<const Component0>(e);
        ASSERT_NE(ptr, nullptr);
        ASSERT_EQ(ptr->value, 1);
    }

    ASSERT_EQ(update_job.count, entities_created.size());
    update_job.count = 0u;
    world.update();
    update_job.run(world, mustache::JobRunMode::kParallel);
    ASSERT_EQ(update_job.count, 0u);



    update(64, 16);

    update(256, 16);
//
    update(1024, 16);
//
    update(2048, 64);

    update_job.show_up = true;
    update_job.run(world, mustache::JobRunMode::kParallel);
    ASSERT_EQ(update_job.count, expected_count);
    for (auto e : entities_created) {
        auto ptr = entities.getComponent<const Component0>(e);
        ASSERT_NE(ptr, nullptr);
        if (ptr->value != 1) {
            ASSERT_EQ(ptr->value, 1);
        }
    }
}

TEST(Job, NonTemplate) {

    mustache::NonTemplateJob job;
    job.job_name = "Move job";
    job.component_requests = {
            {mustache::ComponentFactory::instance().registerComponent<Position>(), false, true},
            {mustache::ComponentFactory::instance().registerComponent<Velocity>(), true, true},
            {mustache::ComponentFactory::instance().registerComponent<Orientation>(), true, true},
    };
    uint32_t call_count = 0;
    job.callback = [&call_count](const mustache::NonTemplateJob::ForEachArrayArgs& args) {
        auto* position = static_cast<Position*>(args.components[0]);
        const auto* velocity = static_cast<const Velocity*>(args.components[1]);
        const auto* orientation = static_cast<const Orientation*>(args.components[2]);

        ASSERT_NE(position, nullptr);
        ASSERT_NE(velocity, nullptr);
        ASSERT_NE(orientation, nullptr);

        for (uint32_t i = 0; i < args.count.toInt(); ++i) {
            ASSERT_EQ(orientation[i].x, 1);
            ASSERT_EQ(orientation[i].y, 2);
            ASSERT_EQ(orientation[i].z, 3);

            position[i].x += velocity[i].value * orientation[i].x;
            position[i].y += velocity[i].value * orientation[i].y;
            position[i].z += velocity[i].value * orientation[i].z;
            ++call_count;
        }
    };

    std::vector<mustache::Entity> entities;
    entities.resize(100);
    mustache::World world;
    for (uint32_t i = 0; i < entities.size(); ++i) {
        entities[i] = world.entities().begin()
                .assign<Position>(0u, 0u, 0u)
                .assign<Velocity>(i)
                .assign<Orientation>(1u, 2u, 3u)
        .end();
    }


    job.run(world);

    ASSERT_EQ(call_count, entities.size());
    
    for (uint32_t i = 0; i < entities.size(); ++i) {
        const auto& pos = *world.entities().getComponent<const Position>(entities[i]);
        const auto& vel = *world.entities().getComponent<const Velocity>(entities[i]);
        const auto& orientation = *world.entities().getComponent<const Orientation>(entities[i]);
        ASSERT_EQ(pos.x, i);
        ASSERT_EQ(pos.y, i * 2);
        ASSERT_EQ(pos.z, i * 3);


        ASSERT_EQ(orientation.x, 1);
        ASSERT_EQ(orientation.y, 2);
        ASSERT_EQ(orientation.z, 3);

        ASSERT_EQ(vel.value, i);

    }
}
