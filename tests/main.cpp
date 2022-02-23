#include <gtest/gtest.h>

#include <mustache/utils/profiler.hpp>

int main(int argc, char* argv[]) {
    MUSTACHE_PROFILER_START();
    MUSTACHE_PROFILER_MAIN_THREAD();
    testing::InitGoogleTest(&argc, argv);
    const auto code = RUN_ALL_TESTS();
    MUSTACHE_PROFILER_DUMP("tests.prof")
    return code;
}
