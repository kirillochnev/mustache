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
struct SharedComponent0 : public mustache::TSharedComponentTag<SharedComponent0> {
    uint32_t dead_beef = 0xDEADBEEF;
    uint32_t boobs = 0xB00B5;
    uint32_t bad_babe = 0xBADBABE;
    bool operator==(const SharedComponent0& rhs) const noexcept {
        return  dead_beef == rhs.dead_beef &&
                boobs == rhs.boobs &&
                bad_babe == rhs.bad_babe;
    }

};

TEST(SharedComponent, AssignShared) {


    struct Component0 {
        std::string src = "Component0: hello world";
    };
    struct Component1 {
        std::string src = "Component1: hello world";
    };

    mustache::World world;
    auto& entities = world.entities();
    auto e = entities.create();
    entities.assign<Component0>(e);
    const auto& ptr = entities.assign<SharedComponent0>(e);
    ASSERT_EQ(ptr.dead_beef, 0xDEADBEEF);
    ASSERT_EQ(ptr.boobs, 0xB00B5);
    ASSERT_EQ(ptr.bad_babe, 0xBADBABE);
    ASSERT_EQ(entities.getComponent<Component0>(e)->src, "Component0: hello world");

    auto e1 = entities.create();
    entities.assign<Component0>(e1);
    const auto& ptr1 = entities.assign<SharedComponent0>(e1);
    ASSERT_EQ(ptr1.dead_beef, 0xDEADBEEF);
    ASSERT_EQ(ptr1.boobs, 0xB00B5);
    ASSERT_EQ(ptr1.bad_babe, 0xBADBABE);
    ASSERT_EQ(entities.getComponent<Component0>(e1)->src, "Component0: hello world");

    ASSERT_EQ(&ptr, &ptr1);

    entities.assign<Component1>(e1);
    ASSERT_EQ(ptr1.dead_beef, 0xDEADBEEF);
    ASSERT_EQ(ptr1.boobs, 0xB00B5);
    ASSERT_EQ(ptr1.bad_babe, 0xBADBABE);
    ASSERT_EQ(entities.getComponent<Component0>(e1)->src, "Component0: hello world");
    ASSERT_EQ(entities.getComponent<Component1>(e1)->src, "Component1: hello world");

    ASSERT_EQ(entities.getComponent<SharedComponent0>(e), &ptr);

    uint32_t count = 0;
    bool is_ok = true;
    const auto job_function = [&count, &is_ok, &ptr](const Component0&, const SharedComponent0& v){
        ++count;
        if (&v != &ptr) {
            is_ok = false;
        }
    };

    entities.forEach(job_function);
    ASSERT_EQ(count, 2);
    ASSERT_TRUE(is_ok);
}