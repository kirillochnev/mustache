//#include <mustache/ecs/entity.hpp>
//#include <mustache/ecs/job.hpp>

#include <iostream>
//#include <mustache/ecs/entity_manager.hpp>
//#include <mustache/utils/context.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/non_template_job.hpp>
//#include <mustache/utils/profiler.hpp>
//#include <mustache/ecs/world.hpp>
#include <mustache/utils/benchmark.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <mustache/utils/dispatch.hpp>
#include <mustache/ecs/job.hpp>
#include <mustache/utils/logger.hpp>

namespace {
    struct Position {
        glm::vec3 value;
    };

    struct Velocity {
        float value{1};
    };

    struct Rotation {
        glm::quat orient;
    };

    MUSTACHE_INLINE glm::vec3 forward(const glm::quat& q) {
        return glm::vec3{
                -2.0f * (q.x * q.z + q.w * q.y),
                -2.0f * (q.y * q.z - q.w * q.x),
                -1.0f + 2.0f * (q.x * q.x + q.y * q.y),
        };
    }
}

void iterate500k() {
    enum : uint32_t {
        kNumObjects = 500000,
        kNumIteration = 100,
    };
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype<Position, Velocity, Rotation>();
    {
        for (uint32_t i = 0; i < kNumObjects; ++i) {
            (void) entities.create(archetype);
        }
    }
    constexpr float dt = 1.0f / static_cast<float>(kNumIteration);

    mustache::Benchmark benchmark;

    mustache::Logger{}.info("Task Begin!");

    for (uint32_t i = 0; i < kNumIteration; ++i) {

        benchmark.add([&entities] {
            entities.forEach([](Position& pos, const Velocity& vel, const Rotation& rot) {
                pos.value += dt * vel.value * forward(rot.orient);
            }, mustache::JobRunMode::kParallel);
        });
    }
    benchmark.show();

}
void non_template_iterate500k() {
    enum : uint32_t {
        kNumObjects = 500000,
        kNumIteration = 100,
    };
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype<Position, Velocity, Rotation>();
    {
        for (uint32_t i = 0; i < kNumObjects; ++i) {
            (void) entities.create(archetype);
        }
    }
    constexpr float dt = 1.0f / static_cast<float>(kNumIteration);
    mustache::NonTemplateJob job;

    job.component_requests = {
            {mustache::ComponentFactory::registerComponent<Position>(), false, true},
            {mustache::ComponentFactory::registerComponent<Velocity>(), true, true},
            {mustache::ComponentFactory::registerComponent<Rotation>(), true, true}
    };

    job.callback = [](const mustache::NonTemplateJob::ForEachArrayArgs& args) {
        Position* MUSTACHE_RESTRICT_PTR pos = static_cast<Position*>(args.components[0]);
        const Velocity* MUSTACHE_RESTRICT_PTR vel = static_cast<const Velocity*>(args.components[1]);
        const Rotation* MUSTACHE_RESTRICT_PTR rot = static_cast<const Rotation*>(args.components[2]);
        for (uint32_t i = 0; i < args.count.toInt(); ++i) {
            pos[i].value += dt * vel[i].value * forward(rot[i].orient);
        }
    };

    mustache::Benchmark benchmark;

    mustache::Logger{}.info("Task Begin!");

    for (uint32_t i = 0; i < kNumIteration; ++i) {
        benchmark.add([&world, &job] {
            job.run(world, mustache::JobRunMode::kParallel);
        });
    }
    benchmark.show();

}


int main() {

//    iterate500k();
    non_template_iterate500k();
    return 0;
}
