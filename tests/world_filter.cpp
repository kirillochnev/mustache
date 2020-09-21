#include <gtest/gtest.h>
#include <mustache/ecs/world_filter.hpp>
#include <mustache/utils/benchmark.hpp>

namespace {
    template<size_t, size_t _Size = 256>
    struct Component {
        std::array<std::byte, _Size> data;
    };

    struct WorldKeeper {
        mustache::World world;
        WorldKeeper():
            world(mustache::WorldId::make(0)) {
            auto& entities = world.entities();
            for (uint32_t i = 0; i < 1024 * 1024; ++i) {
                (void) entities.create<Component<0> >();
                (void) entities.create<Component<0>, Component<1> >();
                (void) entities.create<Component<0>, Component<2> >();
                (void) entities.create<Component<0>, Component<3> >();
                (void) entities.create<Component<0>, Component<4> >();
            }
        }
    };

    mustache::World& getWorld() {
        static WorldKeeper world_keeper;
        return world_keeper.world;
    }
}

TEST(WorldFilter, test0) {
    struct TestFilter : public mustache::CustomWorldFilterResult<TestFilter> {
        bool isChunkMatch(mustache::Archetype& archetype, mustache::ChunkIndex index) {
            (void ) archetype;
            (void ) index;
//            std::cout << archetype.componentMask().toString()  << ": " << index.toInt() << std::endl;
            return true;//index.toInt() != 0;
        }
    };

    TestFilter filter;
    filter.apply(getWorld());

    std::cout << "total_entity_count: " << filter.total_entity_count << std::endl;
    for (const auto& arch_info : filter.filtered_archetypes) {
        std::cout << "\t" <<
        arch_info.archetype->componentMask().toString() << " | " << arch_info.entities_count << " entities in " <<
            arch_info.chunks.size() << " chunks" << std::endl;
    }
}
/*
TEST(WorldFilter, component_version) {
    auto& world = getWorld();
    auto& entities = world.entities();
    auto entity = entities.create<Component<0> >();
    ASSERT_EQ(world.version(), mustache::WorldVersion::make(0));
    ASSERT_EQ(world.version(), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity));
    world.update();
    ASSERT_EQ(world.version(), mustache::WorldVersion::make(1));
    ASSERT_EQ(mustache::WorldVersion::make(0), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity));
    world.update();
    entities.getComponent<Component<0> >(entity)->data.data()[0] = std::byte{0};
    ASSERT_EQ(world.version(), entities.getWorldVersionOfLastComponentUpdate<Component<0> >(entity));
}
*/