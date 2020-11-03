#include <gtest/gtest.h>

#include <mustache/ecs/component_mask.hpp>

using namespace mustache;

TEST(ComponentMask, test1) {

    for (uint32_t i = 0; i < 100; ++i) {
        ComponentIndexMask mask;
        std::set<ComponentIndex> indexes_set;
        for (uint32_t j = 0; j < 10; ++j) {
            ComponentIndex index = ComponentIndex::make(static_cast<uint32_t>(rand()) % ComponentIndexMask::maxElementsCount());
            indexes_set.insert(index);
            mask.set(index, true);
        }

        std::vector<ComponentIndex> indexes(indexes_set.begin(), indexes_set.end());
        const auto items = mask.items();
        ASSERT_EQ(indexes, items);

        uint32_t index = 0;
        mask.forEachItem([&index, &indexes](ComponentIndex component_index){
           ASSERT_EQ(component_index, indexes[index]);
            ++index;
        });
    }

}

TEST(ComponentMask, test2) {

    for (uint32_t i = 0; i < 100; ++i) {
        ComponentIndexMask mask;
        std::set<ComponentIndex> indexes_set;
        for (uint32_t j = 0; j < 10; ++j) {
            ComponentIndex index = ComponentIndex::make(static_cast<uint32_t>(rand()) % ComponentIndexMask::maxElementsCount());
            indexes_set.insert(index);
            mask.set(index, true);
        }

        std::vector<ComponentIndex> indexes(indexes_set.begin(), indexes_set.end());
        const auto items = mask.items();
        ASSERT_EQ(indexes, items);
    }

}
