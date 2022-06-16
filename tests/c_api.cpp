#include <gtest/gtest.h>

#include <mustache/c_api.h>
#include <mustache/utils/logger.hpp>

namespace {
    template<typename T>
    TypeInfo makeTypeInfo(const char* name) noexcept {
        TypeInfo result;
        memset(&result, 0, sizeof (TypeInfo));
        result.name = name;
        result.size = sizeof(T);
        result.align = alignof(T);
        result.functions.create = [](void* ptr) {
            new(ptr) T ();
        };
        result.functions.copy = [](void* dest, const void* src) {
            *static_cast<T*>(dest) = *static_cast<const T*>(src);
        };
        result.functions.move_constructor = [](void* dest, void* src) {
            new (dest) T {std::move(*static_cast<T*>(src))};
        };
        result.functions.move = [](void* dest, void* src) {
            *static_cast<T*>(dest) = std::move(*static_cast<T*>(src));
        };
        result.functions.destroy = [](void* ptr) {
            static_cast<T*>(ptr)->~T();
        };
        return result;
    }
    enum : uint32_t {
        kNumObjects = 16 * 1024,
        kNumIteration = 128,
    };
}

TEST(C_API_EntityManager, assign) {
    static uint32_t num_0 = 0;
    static uint32_t num_1 = 0;
    struct UIntComponent0 {
        uint32_t value = 777 + (++num_0);
    };
    struct UIntComponent1 {
        uint32_t value = 999 + (++num_1);
    };
    struct UnusedComponent {

    };

    const auto c0_id = registerComponent(makeTypeInfo<UIntComponent0>("UIntComponent0"));
    const auto c1_id = registerComponent(makeTypeInfo<UIntComponent1>("UIntComponent1"));
    const auto unused_id = registerComponent(makeTypeInfo<UnusedComponent>("UnusedComponent"));
    mustache::Logger{}.info("%d, %d, %d", c0_id, c1_id, unused_id);

    auto world = createWorld(0u);

    std::vector<Entity > entities(kNumObjects);
    auto archetype = getArchetypeByBitsetMask(world, 0u);
    createEntityGroup(world, archetype, entities.data(), kNumObjects);

    for (uint32_t i = 0; i < entities.size(); ++i) {
        mustache::Logger{}.info("...");
        auto e = entities[i];
        mustache::Logger{}.info("%s", hasComponent(world, e, c0_id) ? "true" : "false");
        ASSERT_FALSE(hasComponent(world, e, c0_id));
        mustache::Logger{}.info("%s", hasComponent(world, e, c1_id) ? "true" : "false");
        ASSERT_FALSE(hasComponent(world, e, c1_id));
        mustache::Logger{}.info("%s", hasComponent(world, e, unused_id) ? "true" : "false");
        ASSERT_FALSE(hasComponent(world, e, unused_id));
        mustache::Logger{}.info("...");
        assignComponent(world, e, c0_id, false);
        mustache::Logger{}.info("...");

        ASSERT_TRUE(hasComponent(world, e, c0_id));
        mustache::Logger{}.info("...");
        ASSERT_FALSE(hasComponent(world, e, c1_id));
        mustache::Logger{}.info("...");
        ASSERT_FALSE(hasComponent(world, e, unused_id));
        mustache::Logger{}.info("...");
        ASSERT_EQ(static_cast<const UIntComponent0*>(getComponent(world, e, c0_id, true))->value, 777 + num_0);
        mustache::Logger{}.info("...");

        assignComponent(world, e, c1_id, false);
        mustache::Logger{}.info("...");
        ASSERT_TRUE(hasComponent(world, e, c0_id));
        mustache::Logger{}.info("...");
        ASSERT_TRUE(hasComponent(world, e, c1_id));
        mustache::Logger{}.info("...");
        ASSERT_FALSE(hasComponent(world, e, unused_id));
        mustache::Logger{}.info("...");
        ASSERT_EQ(static_cast<const UIntComponent0*>(getComponent(world, e, c0_id, true))->value, 777 + num_0);
        mustache::Logger{}.info("...");
        ASSERT_EQ(static_cast<const UIntComponent1*>(getComponent(world, e, c1_id, true))->value, 999 + num_1);
        mustache::Logger{}.info("...");
    }
    mustache::Logger{}.info("...");
    destroyWorld(world);
    mustache::Logger{}.info("...");
}

TEST(C_API_Job, iterate_singlethread_with_required_componen) {
    struct {
        uint32_t count = 0;
        uint32_t cur_iteration = 0;
        void reset() {
            cur_iteration = 0;
            count = 0;
        }
    } static static_data;

    static_data.reset();

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

    const auto pos_id = registerComponent(makeTypeInfo<Position>("Position"));
    const auto vel_id = registerComponent(makeTypeInfo<Velocity>("Velocity"));
    const auto rot_id = registerComponent(makeTypeInfo<Orientation>("Orientation"));



    JobDescriptor update_descriptor;
    memset(&update_descriptor, 0, sizeof(JobDescriptor));

    update_descriptor.name = "Update Job";
    update_descriptor.component_info_arr_size = 3;
    update_descriptor.component_info_arr = new JobArgInfo[3]{
            JobArgInfo{pos_id, true, false},
            JobArgInfo{vel_id, true, true},
            JobArgInfo{rot_id, true, true}
    };
    update_descriptor.callback = [](Job*, JobForEachArrayArg* arg) {
        auto pos_arr = static_cast<Position*>(arg->components[0]);
        auto vel_arr = static_cast<const Velocity*>(arg->components[1]);
        auto rot_arr = static_cast<const Orientation*>(arg->components[2]);
        ASSERT_NE(pos_arr, nullptr);
        ASSERT_NE(vel_arr, nullptr);
        ASSERT_NE(rot_arr, nullptr);
        for (uint32_t i = 0; i < arg->array_size; ++i) {
            pos_arr[i].x += vel_arr[i].value * rot_arr[i].x;
            pos_arr[i].y += vel_arr[i].value * rot_arr[i].y;
            pos_arr[i].z += vel_arr[i].value * rot_arr[i].z;
            ++static_data.count;
        }
    };

    JobDescriptor check_descriptor;
    memset(&check_descriptor, 0, sizeof(JobDescriptor));
    check_descriptor.name = "Check Job";
    check_descriptor.callback = [](Job*, JobForEachArrayArg* arg) {
        auto pos_arr = static_cast<const Position*>(arg->components[0]);
        auto vel_arr = static_cast<const Velocity*>(arg->components[1]);
        auto rot_arr = static_cast<const Orientation*>(arg->components[2]);
        ASSERT_NE(pos_arr, nullptr);
        ASSERT_NE(vel_arr, nullptr);
        ASSERT_NE(rot_arr, nullptr);
        for (uint32_t i = 0; i < arg->array_size; ++i) {
            --static_data.count;

            if (pos_arr[i].x != static_data.cur_iteration * vel_arr[i].value * rot_arr[i].x) {
                throw std::runtime_error("invalid position");
            }
            if (pos_arr[i].y != static_data.cur_iteration * vel_arr[i].value * rot_arr[i].y) {
                throw std::runtime_error("invalid position");
            }
            if (pos_arr[i].z != static_data.cur_iteration * vel_arr[i].value * rot_arr[i].z) {
                throw std::runtime_error("invalid position");
            }
        }
    };

    check_descriptor.component_info_arr_size = 3;
    check_descriptor.component_info_arr = new JobArgInfo[3]{
            JobArgInfo{pos_id, true, true},
            JobArgInfo{vel_id, true, true},
            JobArgInfo{rot_id, true, true}
    };

    std::vector<Entity> entities(kNumObjects);

    auto world = createWorld(0);
    auto archetype = getArchetypeByBitsetMask(world, (1ull << pos_id) | (1ull << vel_id) | (1ull << rot_id));
    createEntityGroup(world, archetype, entities.data(), kNumObjects);
    for (uint32_t i = 0; i < kNumObjects; ++i) {
        auto entity = entities[i];
        static_cast<Velocity*>(getComponent(world, entity, vel_id, false))->value = entity % 128;
        *static_cast<Orientation*>(getComponent(world, entity, rot_id, false)) = Orientation{1, 2, 3};
    }

    auto job_update = makeJob(update_descriptor);
    auto job_check = makeJob(check_descriptor);
    for (uint32_t i = 0; i < kNumIteration; ++i) {
        ++static_data.cur_iteration;

        runJob(job_update, world, kCurrentThread);
        ASSERT_EQ(static_data.count, kNumObjects);

        runJob(job_check, world, kCurrentThread);
        ASSERT_EQ(static_data.count, 0);

        for (auto entity : entities) {
            ASSERT_EQ(entity % 128, static_cast<Velocity*>(getComponent(world, entity, vel_id, true))->value);
            const auto& position = *static_cast<const Position*>(getComponent(world, entity, pos_id, true));
            const auto& velocity = *static_cast<const Velocity*>(getComponent(world, entity, vel_id, true));
            const auto& orientation = *static_cast<const Orientation*>(getComponent(world, entity, rot_id, true));
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.x, position.x);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.y, position.y);
            ASSERT_EQ(static_data.cur_iteration * velocity.value * orientation.z, position.z);
        }
    }
    destroyJob(job_check);
    destroyJob(job_update);
    destroyWorld(world);
    delete[] update_descriptor.component_info_arr;
    delete[] check_descriptor.component_info_arr;
}
