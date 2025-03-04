#include <mustache/ecs/ecs.hpp>
#include <mustache/utils/timer.hpp>
#include <mustache/utils/benchmark.hpp>
#include <iostream>

namespace {
    struct Vec3 {
        float x {0.0f};
        float y {0.0f};
        float z {0.0f};
    };
    struct Quaternion {
        float x {0.0f};
        float y {0.0f};
        float z {0.0f};
        float w {1.0f};
    };

    struct Position {
        Vec3 value;
    };

    struct Velocity {
        float value {1};
    };

    struct Rotation {
        Quaternion value;
    };

    MUSTACHE_INLINE Vec3 forward(const Quaternion& q) {
        return Vec3 {
                -2.0f * (q.x * q.z + q.w * q.y),
                -2.0f * (q.y * q.z - q.w * q.x),
                -1.0f + 2.0f * (q.x * q.x + q.y * q.y),
        };
    }
}

void createEntities(mustache::World& world, uint32_t entities_count, uint32_t mode) {
    auto& entities = world.entities();
    auto& archetype = world.entities().getArchetype<Position, Velocity, Rotation>();
    for (uint32_t i = 0; i < entities_count; ++i) {
        switch (mode) {
            case 0:
                (void)world.entities().create(archetype); // fastest way to create
                break;
            case 1:
                (void)world.entities().create<Position, Velocity, Rotation>();
                break;
            case 2:
                // you can initialize components using next form
                (void)world.entities().begin()
                    .assign<Position>(1.0f, 2.0f, 3.0f) // {} initializer used
                    .assign<Velocity>(Velocity{1.0f}) // copy/move constructor
                    .assign<Rotation>() // default constructor (or nothing for trivially default constructible components)
                .end();
                break;
            case 3:
            {   // For better performance you should avoid such way to create entity
                const auto entity = world.entities().create();
                world.entities().assign<Position>(entity, 1.0f, 2.0f, 3.0f); // {} initializer
                world.entities().assign<Velocity>(entity);
                world.entities().assign<Rotation>(entity);
            }
                break;
        }
    }
}
namespace {
    struct Name {std::string value;};
    struct Transform{};
    struct MeshFilter{};
    struct MeshRenderer{};
    struct Collider{};

    using namespace mustache;
    struct Children {
        std::vector<mustache::Entity> value;
        static void clone(const CloneDest<Children>& dest,
                          const CloneSource<Children>& src,
                          CloneEntityMap& map,
                          World& world) {
            new(dest.value) Children{};
            dest.value->value.reserve(src.value->value.size());
            for (const auto& child: src.value->value) {
                dest.value->value.push_back(world.entities().clone(child, map));
            }
        }
    };

    struct Parent {
        mustache::Entity value;
        static void afterClone(CloneDest<Parent>& dest, const CloneEntityMap& map) {
            dest.value->value = map.remap(dest.value->value);
        }
    };

    struct Camera{};
    struct Controller{};
    struct Prefab{};
}


std::string entityName(mustache::World& world, mustache::Entity e) {
    const auto name = world.entities().getComponent<const Name>(e);
    if (name) {
        return name->value + ", id: " + std::to_string(e.id().toInt());
    }
    return "id: " + std::to_string(e.id().toInt());
};

void printH(mustache::World& world, mustache::Entity root, std::string str = "") {
    const auto parent = world.entities().getComponent<const Parent>(root);
    std::cout << str << entityName(world, root);
    if (parent != nullptr) {
        std::cout <<", child of: " << entityName(world, parent->value);
    }
    std::cout << std::endl;
    const auto children = world.entities().getComponent<const Children>(root);
    if (children == nullptr) {
        return;
    }
    str += "\t";
    for (const auto& ch : children->value) {
        printH(world, ch, str);
    }
}

mustache::Entity createCar(mustache::World& world) {
    auto car = world.entities().begin()
        .assign<Name>("Car")
        .assign<Prefab>()
        .assign<Transform>()
        .assign<MeshFilter>()
        .assign<MeshRenderer>()
        .assign<Collider>()
        .assign<Children>()
    .end();

    auto& children = world.entities().getComponent<Children>(car)->value;
    auto camera = world.entities().begin()
        .assign<Name>("Camera")
        .assign<Camera>()
        .assign<Transform>()
        .assign<Parent>(car)
        .assign<Collider>()
    .end();
    children.push_back(camera);

    for (uint32_t i = 0; i < 4; ++i) {
        auto wheel = world.entities().begin()
            .assign<Name>("Wheel")
            .assign<Transform>()
            .assign<Parent>(car)
            .assign<Children>()
            .assign<Collider>()
            .assign<MeshFilter>()
            .assign<MeshRenderer>()
        .end();
        children.push_back(wheel);

        auto prev = wheel;
        for (uint32_t j = 0; j < 3; ++j) {
            auto wheel_collider = world.entities().begin()
                .assign<Name>("WheelCollider")
                .assign<Transform>()
                .assign<Parent>(prev)
                .assign<Collider>()
                .assign<Children>()
                .assign<MeshFilter>()
            .end();

            world.entities().getComponent<Children>(prev)->value.push_back(wheel_collider);
            prev = wheel_collider;
        }
    }
    return car;
}

void prefabTest() {
    mustache::World world;
    auto car = createCar(world);
    auto car2 = world.entities().clone(car);

    printH(world, car);
    printH(world, car2);

    mustache::Benchmark benchmark;
    benchmark.add([&world, car]{
        for (uint32_t i = 0; i < 1000; ++i) {
            world.entities().clone(car);
        }
    }, 10);

    benchmark.show();
}


template<bool _Const>
void getComponentTest(mustache::World& world, const std::vector<Entity>& entities) {
    constexpr auto Safety = mustache::FunctionSafety::kUnsafe;
    using PositionC = typename std::conditional<_Const, const Position, Position>::type;
    using VelocityC = typename std::conditional<_Const, const Velocity, Velocity>::type;
    using RotationC = typename std::conditional<_Const, const Rotation, Rotation>::type;
    for (const auto& entity : entities) {
        (void) world.entities().getComponent<PositionC, Safety>(entity);
        (void) world.entities().getComponent<VelocityC, Safety>(entity);
//        (void) world.entities().getComponent<RotationC, Safety>(entity);
    }
}

template<bool _Const>
void getComponentTest(mustache::World& world, Entity first, uint32_t entity_count) {
    constexpr auto Safety = mustache::FunctionSafety::kUnsafe;
    using PositionC = typename std::conditional<_Const, const Position, Position>::type;
    using VelocityC = typename std::conditional<_Const, const Velocity, Velocity>::type;
    using RotationC = typename std::conditional<_Const, const Rotation, Rotation>::type;
    auto entity = first;
    auto& entities = world.entities();
    auto location = entities.entityLocation(first);
    auto& archetype = entities.getArchetype(location.archetype);
    auto view = archetype.getElementView(location.index);
    for (uint32_t i = 0; i < entity_count; ++i) {
        (void)view.getData<Safety>(ComponentIndex::make(0));
        (void)view.getData<Safety>(ComponentIndex::make(1));
        ++view;
//        (void) entities.getComponent<PositionC, Safety>(entity);
//        (void) entities.getComponent<VelocityC, Safety>(entity);
//        (void) world.entities().getComponent<RotationC, Safety>(entity);
    }
}

void getComponentBenchMain() {
    constexpr bool first_test_const = false;
    constexpr uint32_t iteration_count = 10;
    constexpr uint32_t entity_count = 1000000;
    mustache::Benchmark benchmark;
    mustache::World world;
    auto& archetype = world.entities().getArchetype<Position, Velocity, Rotation>();
    std::vector<mustache::Entity> entities(entity_count);
    for (uint32_t i = 0; i < entity_count; ++i) {
        entities[i] = world.entities().create(archetype);
    }

    benchmark.add([&world, &entities]{
//        getComponentTest<first_test_const>(world, Entity::makeFromValue(0), entity_count);
        getComponentTest<first_test_const>(world, entities);
    }, iteration_count);
    benchmark.show();
    benchmark.reset();

    benchmark.add([&world, &entities]{
//        getComponentTest<!first_test_const>(world, Entity::makeFromValue(0), entity_count);
        getComponentTest<!first_test_const>(world, entities);
    }, iteration_count);
    benchmark.show();
}

int main() {

    getComponentBenchMain();
    if (true) {
        return 0;
    }

    constexpr float dt = 1.0f / 60.0f;
    struct MoveJob : public mustache::PerEntityJob<MoveJob> {
        static void calc(Position& pos, const Velocity& vel, const Rotation& rot) noexcept {
            const float dist = dt * vel.value;
            const auto dir = forward(rot.value);
            pos.value.x += dist * dir.x;
            pos.value.y += dist * dir.y;
            pos.value.z += dist * dir.z;
        }
#if 0
        void forEachArray(Position* pos, const Velocity* vel, const Rotation* rot, mustache::ComponentArraySize size) {
            const uint32_t count = size.toInt() / 4;
            for (uint32_t i = 0; i < count; ++i) {
                calc(pos[0], vel[0], rot[0]);
                calc(pos[1], vel[1], rot[1]);
                calc(pos[2], vel[2], rot[2]);
                calc(pos[3], vel[3], rot[3]);
                pos +=4;
                vel +=4;
                rot +=4;
            }
        }
#else
        void operator() (Position& pos, const Velocity& vel, const Rotation& rot) noexcept {
            const float dist = dt * vel.value;
            const auto dir = forward(rot.value);
            pos.value.x += dist * dir.x;
            pos.value.y += dist * dir.y;
            pos.value.z += dist * dir.z;
        }
#endif
    } move_job;

//    prefabTest();
//    return  0;
    constexpr uint32_t entities_count = 10;


    mustache::World world;
    createEntities(world, 50000, 3);

    const auto run_mode = mustache::JobRunMode::kSingleThread;

    mustache::Benchmark benchmark;
//    benchmark.add([&]{move_job.run(world, run_mode);}, 100);
    benchmark.add([&]{move_job.run(world, run_mode);}, 100);
    benchmark.show();

    return 0;
}
