#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/world.hpp>
#include <gtest/gtest.h>
#include <map>

TEST(EntityManager, create) {
    static std::map<void*, std::string> created_components;
    struct Component0 {
        Component0() {
           created_components.emplace(this, mustache::type_name<std::remove_reference<decltype(*this)>::type>());
        }
        ~Component0() {
            created_components.erase(this);
        }
    };
    struct Component1 {
        Component1() {
            created_components.emplace(this, mustache::type_name<std::remove_reference<decltype(*this)>::type>());
        }
        ~Component1() {
            created_components.erase(this);
        }
    };
    struct Component2 {
    };

    mustache::World world{mustache::WorldId::make(0)};
    std::vector<mustache::Entity> entity_array;
    const auto create_for_arch = [&entity_array, &world](mustache::Archetype& archetype) {
        for (uint32_t i = 0; i < 100; ++i) {
            entity_array.push_back(world.entities().create(archetype));
        }
    };
    auto& arch0 = world.entities().getArchetype<Component0>();
    auto& arch1 = world.entities().getArchetype<Component0, Component1>();
    auto& arch2 = world.entities().getArchetype<Component0, Component1, Component2>();
    auto& arch3 = world.entities().getArchetype<Component2>();

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

    for (auto entity : entity_array) {
        world.entities().destroyNow(entity);
    }
    ASSERT_EQ(created_components.size(), 0);

}
