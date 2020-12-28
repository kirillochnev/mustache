#include <mustache/ecs/world.hpp>
#include <mustache/ecs/job.hpp>

#include <gtest/gtest.h>

TEST(SharedComponent, FunctionInfo) {
    struct Component0 {};
    struct Component1 {};
    struct Component2 {};
    struct SharedComponent0  : public mustache::SharedComponentTag {};

    const auto func0 = [](Component0&, const Component1, Component2*, SharedComponent0&) {

    };

    const auto func1 = [](mustache::Entity, const Component1, Component2*, const SharedComponent0&) {

    };

    const auto func2 = [](Component0&, const Component1, Component2*) {

    };

    using Info0 = mustache::JobFunctionInfo<decltype(func0)>;
    static_assert(Info0::args_count == 4, "invalid args count");
    static_assert(Info0::any_components_count == 4, "invalid total components count");
    static_assert(Info0::components_count == 3, "invalid non-shared components count");
    static_assert(Info0::shared_components_count == 1, "invalid shared components count");

    using Info1 = mustache::JobFunctionInfo<decltype(func1)>;
    static_assert(Info1::args_count == 4, "invalid args count");
    static_assert(Info1::any_components_count == 3, "invalid total components count");
    static_assert(Info1::components_count == 2, "invalid non-shared components count");
    static_assert(Info1::shared_components_count == 1, "invalid shared components count");

    using Info2 = mustache::JobFunctionInfo<decltype(func2)>;
    static_assert(Info2::args_count == 3, "invalid args count");
    static_assert(Info2::any_components_count == 3, "invalid total components count");
    static_assert(Info2::components_count == 3, "invalid non-shared components count");
    static_assert(Info2::shared_components_count == 0, "invalid shared components count");
}
