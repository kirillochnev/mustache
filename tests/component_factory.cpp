#include <gtest/gtest.h>
#include <mustache/ecs/component_factory.hpp>
namespace {
    template<size_t>
    struct Component {

    };
}
TEST(ComponentFactory, ComponentMask) {
    const std::array component_ids_expected {
            mustache::ComponentId::make(0 + mustache::ComponentFactory::nextComponentId().toInt()),
            mustache::ComponentId::make(1 + mustache::ComponentFactory::nextComponentId().toInt()),
            mustache::ComponentId::make(2 + mustache::ComponentFactory::nextComponentId().toInt()),
            mustache::ComponentId::make(3 + mustache::ComponentFactory::nextComponentId().toInt()),
            mustache::ComponentId::make(4 + mustache::ComponentFactory::nextComponentId().toInt())
    };

    const std::array component_ids_actual = {
            mustache::ComponentFactory::registerComponent<Component<0> >(),
            mustache::ComponentFactory::registerComponent<Component<1> >(),
            mustache::ComponentFactory::registerComponent<Component<2> >(),
            mustache::ComponentFactory::registerComponent<Component<3> >(),
            mustache::ComponentFactory::registerComponent<Component<4> >()
    };

    static_assert(component_ids_actual.size() == component_ids_expected.size());
    for (size_t i = 0; i < component_ids_actual.size(); ++i) {
        std::cout << "component_ids_actual[i]: " << component_ids_actual[i].toInt() << " | "
            << "component_ids_expected[i]: " << component_ids_expected[i].toInt() << std::endl;
    }
    ASSERT_EQ(component_ids_actual, component_ids_expected);

    mustache::ComponentMask mask;
    ASSERT_TRUE(mask.isEmpty());
    auto actual_mask = mustache::ComponentFactory::makeMask<>();
    ASSERT_EQ(actual_mask, mask);
    actual_mask = mustache::ComponentFactory::makeMask<Component<0> >();
    mask.add(component_ids_expected[0]);
    ASSERT_EQ(actual_mask, mask);
    mask.add(component_ids_expected[1]);
    actual_mask = mustache::ComponentFactory::makeMask<Component<0>, Component<1> >();
    ASSERT_EQ(actual_mask, mask);
    mask.add(component_ids_expected[1]);
    ASSERT_EQ(actual_mask, mask);
    actual_mask = mustache::ComponentFactory::makeMask<Component<0>, Component<1>, Component<0> >();
    ASSERT_EQ(actual_mask, mask);
    actual_mask = mustache::ComponentFactory::makeMask<Component<0>, Component<1>, Component<2> >();
    ASSERT_FALSE(actual_mask == mask);
    mask.add(component_ids_expected[2]);
    ASSERT_EQ(actual_mask, mask);
    actual_mask = mustache::ComponentFactory::makeMask<Component<0>, Component<1>, Component<2>,  Component<4> >();
    mask.add(component_ids_expected[4]);
    ASSERT_EQ(actual_mask, mask);
    actual_mask = mustache::ComponentFactory::makeMask<Component<1>, Component<0>, Component<3>, Component<2>, Component<4> >();
    mask.add(component_ids_expected[3]);
    ASSERT_EQ(actual_mask, mask);
}

TEST(ComponentFactory, ComponentMaskIsMatch) {
    auto mask_0 = mustache::ComponentFactory::makeMask<Component<0>, Component<1> >();
    auto mask_1 = mustache::ComponentFactory::makeMask<Component<0>, Component<1> >();
    ASSERT_TRUE(mask_0.isMatch(mask_1));
    mask_1 = mustache::ComponentFactory::makeMask<Component<0> >();
    ASSERT_TRUE(mask_0.isMatch(mask_1));
    mask_1 = mustache::ComponentFactory::makeMask<Component<2> >();
    ASSERT_FALSE(mask_0.isMatch(mask_1));
    mask_1 = mustache::ComponentFactory::makeMask<Component<0>, Component<1>, Component<2> >();
    ASSERT_FALSE(mask_0.isMatch(mask_1));
    mask_1 = mustache::ComponentFactory::makeMask<Component<3>, Component<2> >();
    ASSERT_FALSE(mask_0.isMatch(mask_1));
}
