#include <gtest/gtest.h>
#include <mustache/ecs/event_manager.hpp>

TEST(EventManager, test) {
    mustache::MemoryManager memory_manager;
    mustache::EventManager event_manager {memory_manager};
    std::vector<std::uint32_t> rand_value;
    for (uint32_t i = 0; i < 10000; ++i) {
        rand_value.push_back(rand());
    }
    uint32_t position = 0u;
    auto sub = event_manager.subscribe<uint32_t>([&position, &rand_value](uint32_t value){
        ASSERT_EQ(value, rand_value[position]);
        ++position;
    });
    for (auto value : rand_value) {
        event_manager.post(value);
    }
    ASSERT_EQ(position, rand_value.size());
    sub.reset();
    for (uint32_t i = 0; i < 10000; ++i) {
        event_manager.post(i);
        ASSERT_EQ(position, rand_value.size());
    }
}
