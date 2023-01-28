#include <mustache/ecs/ecs.hpp>

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

    struct UnusedComponent {

    };
}

TEST(EntityManager, create) {
    ASSERT_EQ(created_components.size(), 0);
    {
        mustache::World world{mustache::WorldId::make(0)};
        std::vector<mustache::Entity> entity_array;
        const auto create_for_arch = [&entity_array, &world](mustache::Archetype &archetype) {
            for (uint32_t i = 0; i < 100; ++i) {
                entity_array.push_back(world.entities().create(archetype));
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

TEST(EntityManager, create_entities_check_id_version) {
    ASSERT_EQ(created_components.size(), 0);
    {
        const auto world_id_value = rand() % 127;
        const auto world_id = mustache::WorldId::make(world_id_value);
        mustache::World world{world_id};
        auto &entities = world.entities();
        for (uint32_t i = 0; i < 1024; ++i) {
            auto entity = entities.create();
            ASSERT_EQ(entity.id(), mustache::EntityId::make(i));
            ASSERT_EQ(entity.id().toInt(), i);
            ASSERT_EQ(entity.version(), mustache::EntityVersion::make(0));
            ASSERT_EQ(entity.version().toInt(), 0);
            ASSERT_EQ(entity.worldId(), world_id);
            ASSERT_EQ(entity.worldId().toInt(), world_id_value);
        }
    }
    ASSERT_EQ(created_components.size(), 0);
}

TEST(EntityManager, create_and_destroy_entities_check_id_version) {
    ASSERT_EQ(created_components.size(), 0);
    {
        const auto world_id_value = rand() % 127;
        const auto world_id = mustache::WorldId::make(world_id_value);
        mustache::World world{world_id};
        auto &entities = world.entities();
        for (uint32_t i = 0; i < 1024; ++i) {
            auto entity = entities.create();
            ASSERT_EQ(entity.id(), mustache::EntityId::make(0));
            ASSERT_EQ(entity.id().toInt(), 0);
            ASSERT_EQ(entity.version(), mustache::EntityVersion::make(i));
            ASSERT_EQ(entity.version().toInt(), i);
            ASSERT_EQ(entity.worldId(), world_id);
            ASSERT_EQ(entity.worldId().toInt(), world_id_value);
            entities.destroyNow(entity);
        }
    }
    ASSERT_EQ(created_components.size(), 0);
}

TEST(EntityManager, create_and_destroy_entities_check_id_version2) {
    ASSERT_EQ(created_components.size(), 0);
    {
        const auto world_id_value = rand() % 127;
        const auto world_id = mustache::WorldId::make(world_id_value);
        mustache::World world{world_id};
        auto &entities = world.entities();
        for (uint32_t i = 0; i < 1024; ++i) {
            auto entity0 = entities.create();
            ASSERT_EQ(entity0.id(), mustache::EntityId::make(0));
            ASSERT_EQ(entity0.id().toInt(), 0);
            ASSERT_EQ(entity0.version(), mustache::EntityVersion::make(i));
            ASSERT_EQ(entity0.version().toInt(), i);
            ASSERT_EQ(entity0.worldId(), world_id);
            ASSERT_EQ(entity0.worldId().toInt(), world_id_value);

            auto entity1 = entities.create();
            ASSERT_EQ(entity1.id(), mustache::EntityId::make(1));
            ASSERT_EQ(entity1.id().toInt(), 1);
            ASSERT_EQ(entity1.version(), mustache::EntityVersion::make(i));
            ASSERT_EQ(entity1.version().toInt(), i);
            ASSERT_EQ(entity1.worldId(), world_id);
            ASSERT_EQ(entity1.worldId().toInt(), world_id_value);

            entities.destroyNow(entity1);
            entities.destroyNow(entity0);
        }
    }
    ASSERT_EQ(created_components.size(), 0);
}

TEST(EntityManager, create_and_destroy_entities_check_id_version3) {
    ASSERT_EQ(created_components.size(), 0);
    {
        const auto world_id_value = rand() % 127;
        const auto world_id = mustache::WorldId::make(world_id_value);
        mustache::World world{world_id};
        auto &entities = world.entities();
        for (uint32_t i = 0; i < 1024; ++i) {
            auto entity0 = entities.create();
            ASSERT_EQ(entity0.id(), mustache::EntityId::make(i % 2));
            ASSERT_EQ(entity0.id().toInt(), i % 2);
            ASSERT_EQ(entity0.version(), mustache::EntityVersion::make(i));
            ASSERT_EQ(entity0.version().toInt(), i);
            ASSERT_EQ(entity0.worldId(), world_id);
            ASSERT_EQ(entity0.worldId().toInt(), world_id_value);

            auto entity1 = entities.create();
            ASSERT_EQ(entity1.id(), mustache::EntityId::make((i +1) % 2));
            ASSERT_EQ(entity1.id().toInt(), (i + 1) % 2);
            ASSERT_EQ(entity1.version(), mustache::EntityVersion::make(i));
            ASSERT_EQ(entity1.version().toInt(), i);
            ASSERT_EQ(entity1.worldId(), world_id);
            ASSERT_EQ(entity1.worldId().toInt(), world_id_value);

            entities.destroyNow(entity0);
            entities.destroyNow(entity1);
        }
        ASSERT_EQ(created_components.size(), 0);
    }
    ASSERT_EQ(created_components.size(), 0);
}
TEST(EntityManager, clearArchetype) {
    ASSERT_EQ(created_components.size(), 0);
    {
        static constexpr uint32_t kItemPerArchetype = 5;
        mustache::World world{mustache::WorldId::make(0)};
        std::vector<mustache::Entity> entity_array;
        const auto create_for_arch = [&entity_array, &world](mustache::Archetype& archetype) {
            for (uint32_t i = 0; i < kItemPerArchetype; ++i) {
                entity_array.push_back(world.entities().create(archetype));
            }
        };
        auto &arch0 = world.entities().getArchetype<ComponentWithCheck<0> >();
        auto &arch1 = world.entities().getArchetype<ComponentWithCheck<0>, ComponentWithCheck<1> >();
        auto &arch2 = world.entities().getArchetype<ComponentWithCheck<0>, ComponentWithCheck<1>, PodComponent<0> >();
        auto &arch3 = world.entities().getArchetype<PodComponent<0> >();

        create_for_arch(arch0);
        ASSERT_EQ(created_components.size(), kItemPerArchetype);

        create_for_arch(arch1);
        ASSERT_EQ(created_components.size(), 3 * kItemPerArchetype); // 100 for Component1 and 100 for Component2

        create_for_arch(arch2);
        ASSERT_EQ(created_components.size(), 5 * kItemPerArchetype); // 100 for Component1 and 100 for Component2

        create_for_arch(arch3);
        ASSERT_EQ(created_components.size(), 5 * kItemPerArchetype);

        world.entities().clearArchetype(arch0);
        ASSERT_EQ(created_components.size(), 4 * kItemPerArchetype);

        world.entities().clearArchetype(arch1);
        ASSERT_EQ(created_components.size(), 2 * kItemPerArchetype);

        world.entities().clearArchetype(arch2);
        ASSERT_EQ(created_components.size(), 0);

        world.entities().clearArchetype(arch3);
        ASSERT_EQ(created_components.size(), 0);
    }
    ASSERT_EQ(created_components.size(), 0);

}

TEST(EntityManager, destroyNow) {
    ASSERT_EQ(created_components.size(), 0);
    {
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
        auto &arch1 = world.entities().getArchetype<ComponentWithCheck<0>, ComponentWithCheck<1> >();
        auto &arch2 = world.entities().getArchetype<ComponentWithCheck<0>, ComponentWithCheck<1>, PodComponent<0> >();
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
    ASSERT_EQ(created_components.size(), 0);
}

TEST(EntityManager, destructor) {
    ASSERT_EQ(created_components.size(), 0);

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
    ASSERT_EQ(created_components.size(), 0);
    {
        static uint32_t num_0 = 0;
        static uint32_t num_1 = 0;
        struct UIntComponent0 {
            uint32_t value = 777 + (++num_0);
        };
        struct UIntComponent1 {
            uint32_t value = 999 + (++num_1);
        };
        mustache::World world{mustache::WorldId::make(0)};
        auto &entities = world.entities();

        for (uint32_t i = 0; i < 100; ++i) {
            auto e = entities.create();

            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));
            entities.assign<UIntComponent0>(e);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 777 + num_0);

            entities.assign<UIntComponent1>(e);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 999 + num_1);
            ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 777 + num_0);
        }
    }
    ASSERT_EQ(created_components.size(), 0);
}

TEST(EntityManager, assign_with_custom_constructor) {
    ASSERT_EQ(created_components.size(), 0);
    {
        struct UIntComponent0 {
            uint32_t value = 777;
        };
        struct UIntComponent1 {
            uint32_t value = 999;
        };
        mustache::World world{mustache::WorldId::make(0)};
        auto &entities = world.entities();

        for (uint32_t i = 0; i < 100; ++i) {
            auto e = entities.create();

            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));
            entities.assign<UIntComponent0>(e, UIntComponent0{123u + i});
            ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 123 + i);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));

            entities.assign<UIntComponent1>(e, 321u + i);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));

            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 321u + i);
            ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 123u + i);
        }
    }
    ASSERT_EQ(created_components.size(), 0);

}

TEST(EntityManager, removeComponent) {
    ASSERT_EQ(created_components.size(), 0);
    {
        static uint32_t num_0 = 0;
        static uint32_t num_1 = 0;
        struct UIntComponent0 {
            uint32_t value = 777 + (++num_0);
        };
        struct UIntComponent1 {
            uint32_t value = 999 + (++num_1);
        };
        mustache::World world{mustache::WorldId::make(0)};
        auto& entities = world.entities();

        for (uint32_t i = 0; i < 100; ++i) {
            auto e = entities.create();

            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
            entities.assign<UIntComponent0>(e);
            entities.assign<UIntComponent1>(e);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));

            entities.assign<UnusedComponent>(e);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));

            entities.removeComponent<UIntComponent0>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));

            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 999 + num_1);

            entities.removeComponent<UIntComponent0>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 999 + num_1);

            entities.removeComponent<UIntComponent1>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));

            entities.assign<UIntComponent0>(e, 12345u);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 12345u);
        }

        auto e = entities.create();
        for (uint32_t i = 0; i < 100; ++i) {

            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
            entities.assign<UIntComponent0>(e, UIntComponent0{777});
            entities.assign<UIntComponent1>(e, UIntComponent1{543});
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 777);
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 543);

            entities.removeComponent<UIntComponent0>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 543);

            entities.removeComponent<UIntComponent0>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 543);

            entities.removeComponent<UIntComponent1>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
        }
    }
    ASSERT_EQ(created_components.size(), 0);
}

TEST(EntityManager, move) {
    ASSERT_EQ(created_components.size(), 0);
    {
        static bool c0_active = false;
        static bool c1_active = false;
        struct Component0 {
            Component0() = default;

            Component0(Component0 &&) = default;

            Component0 &operator=(Component0 &&) = default;

            Component0(std::unique_ptr<uint32_t> &&p) :
                    ptr{std::move(p)} {
                if (c0_active) {
                    throw std::runtime_error("Invalid value");
                }
                c0_active = true;
            }

            ~Component0() {
                if (ptr) {
                    c0_active = false;
                }
            }

            std::unique_ptr<uint32_t> ptr;
        };

        struct Component1 {
            Component1 &operator=(Component1 &&) = default;

            Component1() = default;

            Component1(std::unique_ptr<uint32_t> &&p) :
                    ptr{std::move(p)} {
                if (c1_active) {
                    throw std::runtime_error("Invalid value");
                }
                c1_active = true;
            }

            ~Component1() {
                if (ptr) {
                    c1_active = false;
                }
            }

            std::unique_ptr<uint32_t> ptr;
        };

        mustache::World world{mustache::WorldId::make(0)};
        auto &entities = world.entities();
        for (uint32_t i = 0; i < 1024; ++i) {
            auto e = entities.create();
            entities.assign<Component0>(e, std::make_unique<uint32_t>(1));
            entities.assign<Component1>(e, std::make_unique<uint32_t>(123));
            ASSERT_TRUE(entities.hasComponent<Component0>(e));
            ASSERT_TRUE(c0_active);
            ASSERT_TRUE(entities.hasComponent<Component1>(e));
            ASSERT_TRUE(c1_active);

            ASSERT_EQ(*entities.getComponent<Component0>(e)->ptr, 1);
            ASSERT_TRUE(c0_active);
            ASSERT_EQ(*entities.getComponent<Component1>(e)->ptr, 123);
            ASSERT_TRUE(c0_active);

            entities.destroyNow(e);
            ASSERT_FALSE(c0_active);
            ASSERT_FALSE(c1_active);
        }
    }
    ASSERT_EQ(created_components.size(), 0);
}

TEST(EntityManager, remove_not_last) {
    const auto kCount = 1024u;
    struct C0 {
        C0() = default;
        C0(uint64_t v):
                ptr{new uint64_t (v)} {

        }
        std::unique_ptr<uint64_t> ptr;
        bool isMatch(uint64_t v) const noexcept {
            if (!(ptr && v == *ptr)) {
                return false;
            }
            return ptr && v == *ptr;
        }
    };

    struct C1 {
        mustache::Entity entity;
    };

    struct C2 : public PodComponent<16 * 1024>{};

    using namespace mustache;

    World world{WorldId::make(0)};
    auto& entities = world.entities();

    std::vector<Entity> entities_arr;

    for (uint32_t i = 0; i < kCount; ++i) {
        auto e = entities.create();
        entities.assign<C0>(e, e.value);
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));

        entities.assign<C1>(e, e);
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.hasComponent<C1>(e));
        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));
        ASSERT_EQ(entities.getComponent<C1>(e)->entity, e);

        entities.assign<C2>(e);
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.hasComponent<C1>(e));
        ASSERT_TRUE(entities.hasComponent<C2>(e));

        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));
        ASSERT_EQ(entities.getComponent<C1>(e)->entity, e);
        entities_arr.push_back(e);
    }

    for (auto e : entities_arr) {
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.hasComponent<C1>(e));
        ASSERT_TRUE(entities.hasComponent<C2>(e));

        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));
        ASSERT_EQ(entities.getComponent<C1>(e)->entity, e);
    }

    for (uint32_t i = 0; i < kCount; ++i) {
        auto e = entities_arr[i];
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.hasComponent<C1>(e));
        ASSERT_TRUE(entities.hasComponent<C2>(e));

        ASSERT_EQ(entities.getComponent<C1>(e)->entity, e);
        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));
        if (i % 2 == 0) {
            entities.removeComponent<C0>(e);
            ASSERT_FALSE(entities.hasComponent<C0>(e));
            ASSERT_TRUE(entities.hasComponent<C1>(e));
            ASSERT_TRUE(entities.hasComponent<C2>(e));
        }
        if (i % 3 == 0) {
            entities.removeComponent<C1>(e);
            ASSERT_FALSE(entities.hasComponent<C1>(e));
            ASSERT_TRUE(entities.hasComponent<C2>(e));
        }
        if (i % 4 == 0) {
            entities.removeComponent<C2>(e);
            ASSERT_FALSE(entities.hasComponent<C0>(e));
            ASSERT_FALSE(entities.hasComponent<C2>(e));
        }
    }
}

TEST(EntityManager, component_builder) {
    struct Component0 {
        Component0(uint32_t v = 0):
            value {v} {

        }
        uint32_t value;
    };
    struct Component1 {
        Component1(uint32_t v = 0):
        value {v} {

        }
        uint32_t value;
    };
    struct Component2 {
        Component2(uint32_t v = 0):
        value {v} {

        }
        uint32_t value;
    };
    mustache::World world;
    auto& entities = world.entities();
    auto entity = entities.begin().assign<Component0>(0u).assign<Component1>(1u).assign<Component2>(2u).end();
    ASSERT_EQ(entities.getComponent<Component0>(entity)->value, 0);
    ASSERT_EQ(entities.getComponent<Component1>(entity)->value, 1);
    ASSERT_EQ(entities.getComponent<Component2>(entity)->value, 2);

    entity = entities.begin().end();
    ASSERT_FALSE(entities.hasComponent<Component0>(entity));
    ASSERT_FALSE(entities.hasComponent<Component1>(entity));
    ASSERT_FALSE(entities.hasComponent<Component2>(entity));

}

TEST(EntityManager, component_builder_to_modify) {
    struct Component0 {
        Component0(uint32_t v = 0):
                value {v} {

        }
        uint32_t value;
    };
    struct Component1 {
        Component1(uint32_t v = 0):
                value {v} {

        }
        uint32_t value;
    };
    struct Component2 {
        Component2(uint32_t v = 0):
                value {v} {

        }
        uint32_t value;
    };
    struct Component3 {
        Component3(uint32_t v = 0):
        value {v} {

        }
        uint32_t value;
    };
    mustache::World world;
    auto& entities = world.entities();
    const auto entity = entities.begin().assign<Component0>(0u).assign<Component1>(1u).end();
    ASSERT_EQ(entities.getComponent<Component0>(entity)->value, 0);
    ASSERT_EQ(entities.getComponent<Component1>(entity)->value, 1);
    ASSERT_FALSE(entities.hasComponent<Component2>(entity));
    ASSERT_FALSE(entities.hasComponent<Component3>(entity));


    entities.begin(entity)
        .assign<Component2>(2u)
        .assign<Component3>(3u)
        .remove<Component0>()
    .end();

    ASSERT_FALSE(entities.hasComponent<Component0>(entity));
    ASSERT_TRUE(entities.hasComponent<Component1>(entity));
    ASSERT_TRUE(entities.hasComponent<Component2>(entity));
    ASSERT_TRUE(entities.hasComponent<Component3>(entity));
    ASSERT_EQ(entities.getComponent<Component1>(entity)->value, 1);
    ASSERT_EQ(entities.getComponent<Component2>(entity)->value, 2);
    ASSERT_EQ(entities.getComponent<Component3>(entity)->value, 3);

    entities.begin(entity).remove<Component3>().end();
    ASSERT_FALSE(entities.hasComponent<Component0>(entity));
    ASSERT_TRUE(entities.hasComponent<Component1>(entity));
    ASSERT_TRUE(entities.hasComponent<Component2>(entity));
    ASSERT_FALSE(entities.hasComponent<Component3>(entity));
    ASSERT_EQ(entities.getComponent<Component1>(entity)->value, 1);
    ASSERT_EQ(entities.getComponent<Component2>(entity)->value, 2);
}

TEST(EntityManager, archetype_chunk_size) {
    struct Component0 {

    };
    struct Component1 {

    };
    struct Component2 {

    };
    mustache::World world;
    auto& entities = world.entities();
    entities.addChunkSizeFunction<Component0, Component1, Component2>(32, 32);
    auto capacity = entities.getArchetype<Component0, Component1, Component2>().chunkCapacity().toInt();
    ASSERT_EQ(capacity, 32);
    capacity = entities.getArchetype<Component0>().chunkCapacity().toInt();
    ASSERT_EQ(capacity, 1024);

    entities.addChunkSizeFunction<Component1>(16, 16);
    capacity = entities.getArchetype<Component0, Component1>().chunkCapacity().toInt();
    ASSERT_EQ(capacity, 16);
    capacity = entities.getArchetype<Component1, Component2>().chunkCapacity().toInt();
    ASSERT_EQ(capacity, 16);
}

TEST(EntityManager, dependency) {
    struct MainComponent {
        uint32_t value = 0xABADBABE;
    };

    struct DependentComponent {
        uint32_t value = 0xB00B5;
    };

    struct Component0 {
        uint32_t value = 0xDEADBEEF;
    };

    mustache::World world;
    auto& entities = world.entities();

    entities.addDependency<MainComponent, DependentComponent>();

    for (uint32_t i = 0; i < 1000; ++i) {
        auto e = entities.create<DependentComponent>();
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 0xB00B5);
        ASSERT_FALSE(entities.hasComponent<MainComponent>(e));
        ASSERT_FALSE(entities.hasComponent<Component0>(e));

        entities.getComponent<DependentComponent>(e)->value = 777;

        entities.assign<MainComponent>(e);
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 777);
        ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
        ASSERT_EQ(entities.getComponent<MainComponent>(e)->value, 0xABADBABE);
        ASSERT_FALSE(entities.hasComponent<Component0>(e));

        entities.assign<Component0>(e);
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 777);
        ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
        ASSERT_EQ(entities.getComponent<MainComponent>(e)->value, 0xABADBABE);
        ASSERT_TRUE(entities.hasComponent<Component0>(e));
        ASSERT_EQ(entities.getComponent<Component0>(e)->value, 0xDEADBEEF);
    }


    for (uint32_t i = 0; i < 1000; ++i) {
        auto e = entities.create<MainComponent>();
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 0xB00B5);
        ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
        ASSERT_EQ(entities.getComponent<MainComponent>(e)->value, 0xABADBABE);
        ASSERT_FALSE(entities.hasComponent<Component0>(e));

        entities.assign<Component0>(e);
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 0xB00B5);
        ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
        ASSERT_EQ(entities.getComponent<MainComponent>(e)->value, 0xABADBABE);
        ASSERT_TRUE(entities.hasComponent<Component0>(e));
        ASSERT_EQ(entities.getComponent<Component0>(e)->value, 0xDEADBEEF);
    }

    for (uint32_t i = 0; i < 1000; ++i) {
        auto e = entities.create<Component0>();
        ASSERT_TRUE(entities.hasComponent<Component0>(e));
        ASSERT_EQ(entities.getComponent<Component0>(e)->value, 0xDEADBEEF);

        ASSERT_FALSE(entities.hasComponent<MainComponent>(e));
        ASSERT_FALSE(entities.hasComponent<DependentComponent>(e));

        entities.assign<MainComponent>(e, MainComponent{123});
        ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
        ASSERT_EQ(entities.getComponent<MainComponent>(e)->value, 123);
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 0xB00B5);

        ASSERT_TRUE(entities.hasComponent<Component0>(e));
        ASSERT_EQ(entities.getComponent<Component0>(e)->value, 0xDEADBEEF);

        entities.removeComponent<DependentComponent>(e); // nothing will happend. DependentComponent can not be removed before MainComponent
        ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
        ASSERT_EQ(entities.getComponent<MainComponent>(e)->value, 123);
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 0xB00B5);

        ASSERT_TRUE(entities.hasComponent<Component0>(e));
        ASSERT_EQ(entities.getComponent<Component0>(e)->value, 0xDEADBEEF);
    }

    for (uint32_t i = 0; i < 1000; ++i) {
        auto e = entities.create<Component0, MainComponent>();
        ASSERT_TRUE(entities.hasComponent<Component0>(e));
        ASSERT_EQ(entities.getComponent<Component0>(e)->value, 0xDEADBEEF);
        ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
        ASSERT_EQ(entities.getComponent<MainComponent>(e)->value, 0xABADBABE);
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 0xB00B5);
    }

    for (uint32_t i = 0; i < 1000; ++i) {
        auto e = entities.begin().assign<Component0>(Component0{123}).assign<MainComponent>(MainComponent{777}).end();
        ASSERT_TRUE(entities.hasComponent<Component0>(e));
        ASSERT_EQ(entities.getComponent<Component0>(e)->value, 123);
        ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
        ASSERT_EQ(entities.getComponent<MainComponent>(e)->value, 777);
        ASSERT_TRUE(entities.hasComponent<DependentComponent>(e));
        ASSERT_EQ(entities.getComponent<DependentComponent>(e)->value, 0xB00B5);
    }

}

TEST(EntityManager, indirect_dependency) {
    struct MainComponent {
        uint32_t value = 0xDEADBEEF;
    };
    struct Component0 {
        uint32_t value = 0xDEADBEEF;
    };
    struct Component1 {
        uint32_t value = 0xDEADBEEF;
    };

    mustache::World world;
    auto& entities = world.entities();

    auto e = entities.begin().assign<MainComponent>().end();
    ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
    ASSERT_FALSE(entities.hasComponent<Component0>(e));
    ASSERT_FALSE(entities.hasComponent<Component1>(e));
    ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));

    entities.addDependency<MainComponent, Component0>();
    e = entities.begin().assign<MainComponent>().end();
    ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
    ASSERT_TRUE(entities.hasComponent<Component0>(e));
    ASSERT_FALSE(entities.hasComponent<Component1>(e));
    ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));

    entities.addDependency<Component0, Component1>();
    e = entities.begin().assign<Component0>().end();
    ASSERT_FALSE(entities.hasComponent<MainComponent>(e));
    ASSERT_TRUE(entities.hasComponent<Component0>(e));
    ASSERT_TRUE(entities.hasComponent<Component1>(e));
    ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));

    e = entities.begin().assign<MainComponent>().end();
    ASSERT_TRUE(entities.hasComponent<MainComponent>(e));
    ASSERT_TRUE(entities.hasComponent<Component0>(e));
    ASSERT_TRUE(entities.hasComponent<Component1>(e));
    ASSERT_FALSE(entities.hasComponent<UnusedComponent>(e));
}

TEST(EntityManager, custom_constructor) {
    struct CreateWithWorldAndEntity{
        CreateWithWorldAndEntity(mustache::World&, mustache::Entity e):
            entity{e} {

        }
        mustache::Entity entity;
    };
    struct CreateWithEntityAndWorld{
        CreateWithEntityAndWorld(mustache::Entity e, mustache::World&):
                entity{e} {

        }
        mustache::Entity entity;
    };
    struct CreateWithEntity {
        CreateWithEntity(mustache::Entity e):
                entity{e} {

        }
        mustache::Entity entity;
    };
    struct CreateWithWorld {
        CreateWithWorld(mustache::World& w):
            world{&w} {

        }
        mustache::World* world;
    };
    mustache::World world;
    for (uint32_t i = 0; i < 10000; ++i) {
        auto entity = world.entities().create();
        if (i % 2) {
            world.entities().assign<CreateWithWorld>(entity);
        }
        if (i % 3) {
            world.entities().assign<CreateWithWorldAndEntity>(entity);
        }
        if (i % 5) {
            world.entities().assign<CreateWithEntity>(entity);
        }
        if (i % 7) {
            world.entities().assign<CreateWithEntityAndWorld>(entity);
        }
    }
    world.entities().begin()
        .assign<CreateWithEntityAndWorld>()
        .assign<CreateWithEntity>()
    .end();
    auto& archetype = world.entities().getArchetype<CreateWithEntity, CreateWithEntityAndWorld,
                                                    CreateWithWorld, CreateWithWorldAndEntity>();
    (void) world.entities().create(archetype);
    world.entities().forEach([](mustache::World& world, mustache::Entity entity, const CreateWithEntity* e,
                                        const CreateWithEntityAndWorld* ew, const CreateWithWorld* w,
                                        const CreateWithWorldAndEntity* we) {
        if (w) {
            ASSERT_EQ(w->world, &world);
        }
        if (e) {
            ASSERT_EQ(e->entity, entity);
        }
        if (we) {
            ASSERT_EQ(we->entity, entity);
        }
        if (ew) {
            ASSERT_EQ(ew->entity, entity);
        }
    });
}
