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
#include <mustache/utils/logger.hpp>

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

void create1m() {
    enum : uint32_t {
        kNumObjects = 1000000,
        kNumIteration = 100,
    };

    mustache::World world{mustache::WorldId::make(0)};
    mustache::ComponentFactory::registerComponent<Rotation>();
    mustache::ComponentFactory::registerComponent<Position>();
    mustache::ComponentFactory::registerComponent<Velocity>();
    mustache::ComponentIdMask mask;
    mask.add(mustache::ComponentId::make(0));
    mask.add(mustache::ComponentId::make(1));
    mask.add(mustache::ComponentId::make(2));
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype(mask);
    mustache::Benchmark benchmark;
    for(uint32_t i = 0; i < kNumIteration; ++i) {
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
        kNumIteration = 100,
    };
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype<Position, Velocity, Rotation>();
    for (uint32_t i = 0; i < kNumObjects; ++i) {
        (void)entities.create(archetype);
    }

    constexpr float dt = 1.0f / kNumIteration;

    mustache::Benchmark benchmark;

    mustache::Logger{}.info("Task Begin!");
//    struct alignas(128) AlignedUint {
//        uint32_t value = 0;
//    };
//    std::vector<AlignedUint> count_arr;

//    const auto count = [&count_arr]() {
//        uint32_t result = 0;
//        for (auto i : count_arr) {
//            result += i.value;
//        }
//        return result;
//    };
    for (uint32_t i = 0; i < kNumIteration; ++i) {
//        count_arr.clear();
//        count_arr.resize(world.dispatcher().threadCount() + 1);
        benchmark.add([&entities/*, &count_arr*/] {
            entities.forEach([/*&count_arr*/](Position& pos, const Velocity& vel, const Rotation& rot/*,
                    const mustache::JobInvocationIndex& invocation_index*/) {
                pos.value += dt * vel.value * forward(rot.orient);
//                ++count_arr[invocation_index.thread_id.toInt()].value;
            }, mustache::JobRunMode::kParallel);
        });

        /*if (count() != kNumObjects) {
            throw std::runtime_error("Invalid num objects");
        }*/
    }
    benchmark.show();

}

void createEmptyAndAssign() {
    enum : uint32_t {
        kNumObjects = 100000,
        kNumIteration = 100,
    };

    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();
    mustache::Benchmark benchmark;
    for(uint32_t i = 0; i < kNumIteration; ++i) {
        benchmark.add([&] {
            for (uint32_t j = 0; j < kNumObjects; ++j) {
                auto e = entities.create();
                entities.assign<Velocity>(e);
                entities.assign<Position>(e);
                entities.assign<Rotation>(e);
            }
        });
        entities.clear();
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

void testGlobalIndex();
void POC();
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
//    createEmptyAndAssign();

    iterate500k();
//    testGlobalIndex();

//    POC();
    return 0;
}
