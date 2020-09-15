#include <gtest/gtest.h>
#include <mustache/utils/type_info.hpp>


struct Component0
{

};

struct MyStruct
{

};

struct Foo
{

};

template<typename T>
class TemplatedFoo {

};

void fooFunc(MyStruct&) {

}
TEST(TypeInfo, type_name) {
    EXPECT_EQ(mustache::type_name<Foo>(), "Foo");
    EXPECT_EQ(mustache::type_name<Component0>(), "Component0");
    EXPECT_EQ(mustache::type_name<MyStruct>(), "MyStruct");
    EXPECT_EQ(mustache::type_name<TemplatedFoo<int> >(), "TemplatedFoo<int>");
    EXPECT_EQ(mustache::type_name<const TemplatedFoo<int> >(), "const TemplatedFoo<int>");
    EXPECT_EQ(mustache::type_name<int>(), "int");
    EXPECT_EQ(mustache::type_name<float>(), "float");
    EXPECT_EQ(mustache::type_name<const float>(), "const float");
}