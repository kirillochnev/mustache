#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/world.hpp>
#include <gtest/gtest.h>
#include <map>

namespace {
    std::map<void*, std::string> created_components;

    template<size_t _I>
    struct ComponentWithCheck {
        ComponentWithCheck() {
            created_components.emplace(this, "ComponentWithCheck" + std::to_string(_I));
        }
        ~ComponentWithCheck() {
            created_components.erase(this);
        }
    };
    template<size_t>
    struct PodComponent {

    };
}

TEST(EntityManager, create) {
    mustache::World world{mustache::WorldId::make(0)};
    std::vector<mustache::Entity> entity_array;
    const auto create_for_arch = [&entity_array, &world](mustache::Archetype &archetype) {
        for (uint32_t i = 0; i < 100; ++i) {
            entity_array.push_back(world.entities().create(archetype));
        }
    };
    auto &arch0 = world.entities().getArchetype<ComponentWithCheck<0> >();
    auto &arch1 = world.entities().getArchetype<ComponentWithCheck<0> , ComponentWithCheck<1> >();
    auto &arch2 = world.entities().getArchetype<ComponentWithCheck<0> , ComponentWithCheck<1> , PodComponent<0> >();
    auto &arch3 = world.entities().getArchetype<PodComponent<0> >();

    create_for_arch(arch0);
    ASSERT_EQ(created_components.size(), 100);

    create_for_arch(arch1);
    ASSERT_EQ(created_components.size(), 300); // 100 for Component1 and 100 for Component2

    create_for_arch(arch2);
    ASSERT_EQ(created_components.size(), 500); // 100 for Component1 and 100 for Component2

    create_for_arch(arch3);
    ASSERT_EQ(created_components.size(), 500);
}


TEST(EntityManager, clearArchetype) {

    mustache::World world{mustache::WorldId::make(0)};
    std::vector<mustache::Entity> entity_array;
    const auto create_for_arch = [&entity_array, &world](mustache::Archetype &archetype) {
        for (uint32_t i = 0; i < 100; ++i) {
            entity_array.push_back(world.entities().create(archetype));
        }
    };
    auto &arch0 = world.entities().getArchetype<ComponentWithCheck<0> >();
    auto &arch1 = world.entities().getArchetype<ComponentWithCheck<0> , ComponentWithCheck<1> >();
    auto &arch2 = world.entities().getArchetype<ComponentWithCheck<0> , ComponentWithCheck<1> , PodComponent<0> >();
    auto &arch3 = world.entities().getArchetype<PodComponent<0> >();

    create_for_arch(arch0);
    ASSERT_EQ(created_components.size(), 100);

    create_for_arch(arch1);
    ASSERT_EQ(created_components.size(), 300); // 100 for Component1 and 100 for Component2

    create_for_arch(arch2);
    ASSERT_EQ(created_components.size(), 500); // 100 for Component1 and 100 for Component2

    create_for_arch(arch3);
    ASSERT_EQ(created_components.size(), 500);

    world.entities().clearArchetype(arch0);
    ASSERT_EQ(created_components.size(), 400);

    world.entities().clearArchetype(arch1);
    ASSERT_EQ(created_components.size(), 200);

    world.entities().clearArchetype(arch2);
    ASSERT_EQ(created_components.size(), 0);

    world.entities().clearArchetype(arch3);
    ASSERT_EQ(created_components.size(), 0);

}

TEST(EntityManager, destroyNow) {

    mustache::World world{mustache::WorldId::make(0)};
    std::vector<std::pair<mustache::Entity, size_t> > entity_array;
    size_t created_components_expected_size = 0;
    const auto create_for_arch = [&](mustache::Archetype &archetype, size_t dsize) {
        for (uint32_t i = 0; i < 100; ++i) {
            created_components_expected_size += dsize;
            entity_array.emplace_back(world.entities().create(archetype), created_components_expected_size);
        }
    };
    auto &arch0 = world.entities().getArchetype<ComponentWithCheck<0> >();
    auto &arch1 = world.entities().getArchetype<ComponentWithCheck<0> , ComponentWithCheck<1> >();
    auto &arch2 = world.entities().getArchetype<ComponentWithCheck<0> , ComponentWithCheck<1> , PodComponent<0> >();
    auto &arch3 = world.entities().getArchetype<PodComponent<0> >();

    create_for_arch(arch0, 1);
    ASSERT_EQ(created_components.size(), 100);
    ASSERT_EQ(created_components_expected_size, 100);

    create_for_arch(arch1, 2);
    ASSERT_EQ(created_components.size(), 300); // 100 for Component1 and 100 for Component2
    ASSERT_EQ(created_components_expected_size, 300);

    create_for_arch(arch2, 2);
    ASSERT_EQ(created_components.size(), 500); // 100 for Component1 and 100 for Component2
    ASSERT_EQ(created_components_expected_size, 500);

    create_for_arch(arch3, 0);
    ASSERT_EQ(created_components.size(), 500);
    ASSERT_EQ(created_components_expected_size, 500);

    for (auto it = entity_array.rbegin(); it != entity_array.rend(); ++it) {
        ASSERT_EQ(it->second, created_components.size());
        world.entities().destroyNow(it->first);
    }

}

TEST(EntityManager, destructor) {
    {
        mustache::World world{mustache::WorldId::make(0)};
        const auto create_for_arch = [&world](mustache::Archetype &archetype) {
            for (uint32_t i = 0; i < 100; ++i) {
                (void)world.entities().create(archetype);
            }
        };
        auto &arch0 = world.entities().getArchetype<ComponentWithCheck<0> >();
        auto &arch1 = world.entities().getArchetype<ComponentWithCheck<0>, ComponentWithCheck<1> >();
        auto &arch2 = world.entities().getArchetype<ComponentWithCheck<0>, ComponentWithCheck<1>, PodComponent<0> >();
        auto &arch3 = world.entities().getArchetype<PodComponent<0> >();

        create_for_arch(arch0);
        ASSERT_EQ(created_components.size(), 100);

        create_for_arch(arch1);
        ASSERT_EQ(created_components.size(), 300); // 100 for Component1 and 100 for Component2

        create_for_arch(arch2);
        ASSERT_EQ(created_components.size(), 500); // 100 for Component1 and 100 for Component2

        create_for_arch(arch3);
        ASSERT_EQ(created_components.size(), 500);
    }
    ASSERT_EQ(created_components.size(), 0);
}

TEST(EntityManager, assign) {
    struct UIntComponent0 {
        uint32_t value = 777;
    };
    struct UIntComponent1 {
        uint32_t value = 999;
    };
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();

    auto e = entities.create();

    entities.assign<UIntComponent0>(e);
    ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 777);

    entities.assign<UIntComponent1>(e);
    ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 999);
    ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 777);
}

TEST(EntityManager, assign_with_custom_constructor) {
    struct UIntComponent0 {
        uint32_t value = 777;
    };
    struct UIntComponent1 {
        uint32_t value = 999;
    };
    mustache::World world{mustache::WorldId::make(0)};
    auto& entities = world.entities();

    auto e = entities.create();

    entities.assign<UIntComponent0>(e, UIntComponent0{123u});
    ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 123);

    entities.assign<UIntComponent1>(e, 321u);
    ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 321u);
    ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 123u);
}