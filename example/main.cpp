#include <mustache/ecs/ecs.hpp>

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

int main() {
    constexpr uint32_t entities_count = 10;
    constexpr float dt = 1.0f / 60.0f;

    mustache::World world;
    createEntities(world, 10, 3);

    const auto run_mode = mustache::JobRunMode::kCurrentThread; // or kParallel

    world.entities().forEach([](Position& pos, const Velocity& vel, const Rotation& rot) {
        const float dist = dt * vel.value;
        const auto dir = forward(rot.value);
        pos.value.x += dist * dir.x;
        pos.value.y += dist * dir.y;
        pos.value.z += dist * dir.z;
    }, run_mode);

    return 0;
}
