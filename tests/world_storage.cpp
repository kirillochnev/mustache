#include <mustache/ecs/world.hpp>

#include <gtest/gtest.h>

using namespace mustache;

TEST(WorldStorage, store_load) {
    World world;
    auto& storage = world.storage();

    for (uint32_t i = 0; i < 1000; ++i) {
        const auto rand_value = rand();
        const auto tag = ObjectTag::fromStr(std::string("tag") + std::to_string(rand_value));
        storage.store<uint32_t>(tag, rand_value);
        ASSERT_EQ(*storage.load<uint32_t>(tag), rand_value);
    }
}

TEST(WorldStorage, singleton) {
    World world;
    auto& storage = world.storage();
    struct Object0 {
        uint32_t value = 0;
    };
    struct Object1 {
        uint32_t value = 0;
    };
    struct Object2 {
        uint32_t value = 0;
    };

    ASSERT_EQ(storage.getInstanceOf<Object0>(), nullptr);
    ASSERT_EQ(storage.getInstanceOf<Object1>(), nullptr);
    ASSERT_EQ(storage.getInstanceOf<Object2>(), nullptr);

    storage.storeSingleton<Object0>(Object0{123});
    ASSERT_EQ(storage.getInstanceOf<Object0>()->value, 123);
    ASSERT_EQ(storage.getInstanceOf<Object1>(), nullptr);
    ASSERT_EQ(storage.getInstanceOf<Object2>(), nullptr);

    storage.storeSingleton<Object1>(Object1{777});
    ASSERT_EQ(storage.getInstanceOf<Object0>()->value, 123);
    ASSERT_EQ(storage.getInstanceOf<Object1>()->value, 777);
    ASSERT_EQ(storage.getInstanceOf<Object2>(), nullptr);

    storage.storeSingleton<Object2>(Object2{2834});
    ASSERT_EQ(storage.getInstanceOf<Object0>()->value, 123);
    ASSERT_EQ(storage.getInstanceOf<Object1>()->value, 777);
    ASSERT_EQ(storage.getInstanceOf<Object2>()->value, 2834);

}
