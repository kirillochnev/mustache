cmake_minimum_required(VERSION 3.10)
project("")
set(CMAKE_CXX_STANDARD 17)

add_library(mustache_profiler INTERFACE)

option(MUSTACHE_BUILD_WITH_EASY_PROFILER "Use easy profiler" OFF)

if (MUSTACHE_BUILD_WITH_EASY_PROFILER)
    target_compile_definitions(mustache_profiler INTERFACE -DMUSTACHE_BUILD_WITH_EASY_PROFILER=1)
    add_subdirectory(easy_profiler)
    set(EASY_PROFILER_NO_GUI ON CACHE BOOL "")
    set(EASY_PROFILER_NO_SAMPLES ON CACHE BOOL "")
    target_link_libraries(mustache_profiler INTERFACE easy_profiler)
else()
    target_compile_definitions(mustache_profiler INTERFACE -DBUILD_WITH_EASY_PROFILER=0)
endif(MUSTACHE_BUILD_WITH_EASY_PROFILER)
