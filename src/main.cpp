//#include <mustache/ecs/entity.hpp>
//#include <mustache/ecs/job.hpp>

#include <iostream>
//#include <mustache/ecs/entity_manager.hpp>
//#include <mustache/utils/context.hpp>
#include <mustache/ecs/component_factory.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/world.hpp>
//#include <mustache/ecs/world.hpp>
#include <mustache/utils/benchmark.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <mustache/utils/dispatch.hpp>
#include <mustache/ecs/job.hpp>

struct Component0 {
    uint32_t i = 0;
};
struct Component1 {
    uint32_t i = 0;
};
struct Component2 {
    uint32_t i = 0;
};
struct Component3 {
    uint32_t i = 0;
};

template<size_t N>
struct TestComponent {
    TestComponent() {
        std::cout << __PRETTY_FUNCTION__  << "   this: " << this << std::endl;
    }
    ~TestComponent() {
        std::cout << __PRETTY_FUNCTION__  << "   this: " << this << std::endl;
    }
    std::array<std::byte, N> data;
};

template<typename T>
void create(std::byte*& ptr, const mustache::ComponentId& id) {
    mustache::ComponentFactory::initComponents(id, ptr, 1);
    ptr += sizeof(T);
}

template<typename T>
void destroy(std::byte*& ptr, const mustache::ComponentId& id) {
    mustache::ComponentFactory::destroyComponents(id, ptr, 1);
    ptr += sizeof(T);
}

namespace {
    struct Position {
        glm::vec3 value = glm::vec3(1,2, 3);
    };

    struct Velocity {
        float value;//{1};
    };

    struct Rotation {
        glm::quat orient;//{glm::vec3{randf(), randf(), randf()}};
    };

    MUSTACHE_INLINE glm::vec3 forward(const glm::quat& q) {
        return glm::vec3{
                -2.0f * (q.x * q.z + q.w * q.y),
                -2.0f * (q.y * q.z - q.w * q.x),
                -1.0f + 2.0f * (q.x * q.x + q.y * q.y),
        };

    }

}

void foo() {
    for (uint32_t i = 0; i < 1000; ++i) {
        mustache::ComponentFactory::registerComponent<TestComponent<0> >();
        mustache::ComponentFactory::registerComponent<Component2>();
        mustache::ComponentFactory::registerComponent<TestComponent<1> >();
        mustache::ComponentFactory::registerComponent<Component0>();
    }
    mustache::ComponentMask mask;
    mask.add(mustache::ComponentId::make(0));
//    mask.add(mustache::ComponentId::make(1));
    mask.add(mustache::ComponentId::make(2));


    mustache::World world{mustache::WorldId::make(0)};
    mustache::Entity entity{0};
    world.entities().getArchetype(mask).insert(entity);

}
void create1m() {
    enum : uint32_t {
        kNumObjects = 1000000,
        kNumIteration = 100,
    };

    mustache::World world{mustache::WorldId::make(0)};
    mustache::ComponentFactory::registerComponent<Rotation>();
    mustache::ComponentFactory::registerComponent<Position>();
    mustache::ComponentFactory::registerComponent<Velocity>();
    mustache::ComponentMask mask;
    mask.add(mustache::ComponentId::make(0));
    mask.add(mustache::ComponentId::make(1));
    mask.add(mustache::ComponentId::make(2));
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype(mask);
    mustache::Benchmark benchmark;
    for(uint32_t i = 0; i < kNumIteration; ++i) {
        const auto& info = archetype.operations();
        const auto component_index = info.componentIndex<Position>();
        benchmark.add([&] {
            for (uint32_t j = 0; j < kNumObjects; ++j) {
                (void)entities.create(archetype);
            }
        });
        entities.clear();
    }
    benchmark.show();
}


void iterate500k() {
    enum : uint32_t {
        kNumObjects = 500000,
        kNumIteration = 1000,
    };
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype<Position, Velocity, Rotation>();
//    auto& archetype2 = entities.getArchetype<Position, Velocity, Rotation, Component0>();
    for (uint32_t i = 0; i < kNumObjects; ++i) {
        (void)entities.create(archetype);
//        (void)entities.create(archetype2);
    }
    constexpr float dt = 1.0f / kNumIteration;
    struct UpdatePosJob : public mustache::PerEntityJob <UpdatePosJob> {
//        uint32_t count = 0;
        std::vector<uint32_t> count;

        uint32_t totalCount() const {
            uint32_t result = 0;
            for (auto i : count) {
                result += i;
            }
            return result;
        }
        /// FIX: uint32_t size is looks like component type, so no archetype match built mask
        /*void forEachArray(uint32_t size, Position* position,
                Velocity* velocity, const Rotation* rotation, mustache::JobInvocationIndex invocation_index) {
            for (uint32_t i = 0; i < size; ++i) {
                position[i].value += dt * forward(rotation[i].orient) * velocity[i].value;
            }
            count[invocation_index.task_index.toInt()] += size;
        }*/
        void operator()(Position& pos, const Velocity& vel, const mustache::RequiredComponent<Rotation>& rot,
                mustache::JobInvocationIndex invocation_index) {
            (void)pos;
            (void)vel;
            (void)rot;

            pos.value += dt * vel.value * forward(rot.ptr->orient);
            ++count[invocation_index.task_index.toInt()];
//            ++count;
        }
    };
    UpdatePosJob update_pos_job;
    mustache::Benchmark benchmark;

    mustache::Dispatcher dispatcher;
    for (uint32_t i = 0; i < kNumIteration; ++i) {
//        update_pos_job.count = 0;
        update_pos_job.count.clear();
        update_pos_job.count.resize(dispatcher.threadCount(), 0);
        benchmark.add([&world, &update_pos_job, &dispatcher] {
//            update_pos_job.run(world, dispatcher);
            update_pos_job.runParallel(world, dispatcher.threadCount(), dispatcher);
        });
        if (update_pos_job.totalCount() != kNumObjects) {
            throw std::runtime_error(std::to_string(update_pos_job.totalCount()) + " vs " + std::to_string(static_cast<uint32_t>(kNumObjects)));
        }
    }
    benchmark.show();
}

void testComponent() {
    struct TestComponent_0 {
        uint32_t rand_value = rand();
        TestComponent_0(){
            std::cout << "TestComponent_0: " << rand_value << std::endl;
        }
    };
    struct TestComponent_1 {
        uint32_t rand_value = rand();
        TestComponent_1(){
            std::cout << "TestComponent_1: " << rand_value << std::endl;
        }
    };

    const auto mask = mustache::ComponentFactory::makeMask<TestComponent_0, TestComponent_1>();
    mustache::World world{mustache::WorldId::make(0)};
    auto& archetype = world.entities().getArchetype(mask);
    auto e0 = world.entities().create(archetype);
    auto e1 = world.entities().create(archetype);

//    const auto show = [&world](mustache::Entity entity) {
//        world.entities().
//    };
}

template<typename T>
void showComponentInfo() {
    using Info = typename mustache::ComponentType<T>::type;
    const auto is_component_required = mustache::ComponentType<T>::is_component_required;
    std::cout << "T name: [" << mustache::type_name<T>() << "] component name: [" << mustache::type_name<Info >() << "]"
            << "\t\t\t| component is " << (is_component_required ? "" : "NOT ") << "required" <<std::endl;
}
int main() {
//    showComponentInfo<Component0>();
//    showComponentInfo<Component0*>();
//    showComponentInfo<const Component0>();
//    showComponentInfo<const Component0*>();
//    showComponentInfo<const Component0* const>();
//    showComponentInfo<const Component0&>();
//    showComponentInfo<Component0&>();
//    showComponentInfo<mustache::RequiredComponent<Component0> >();
//    showComponentInfo<mustache::RequiredComponent<const Component0&> >();
//    showComponentInfo<const mustache::RequiredComponent<Component0>& >();
//    showComponentInfo<const mustache::RequiredComponent<const Component0>& >();
//    showComponentInfo<const mustache::RequiredComponent<const Component0> >();
//    showComponentInfo<mustache::RequiredComponent<const Component0>& >();
//    showComponentInfo<const mustache::RequiredComponent<const Component0> >();


//    foo();
//    create1m();
    iterate500k();
    return 0;
}