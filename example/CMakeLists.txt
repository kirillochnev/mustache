# FIXME: avoid absolute paths and global includes, use target_include_directories
# TODO: add glm dependencies in cmake with FetchContent or use a dependency manager (conan/vcpkg)
include_directories("/home/kirill/Documents/projects/c++/ecs_bench/third_party/glm")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(OTHER_FLAGS "-s TOTAL_MEMORY=1024MB -s WASM=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OTHER_FLAGS}")
endif()

add_executable(mustache_example
        main.cpp
        event_manager_bench.cpp
)

target_link_libraries(mustache_example mustache)



add_executable(example_component_add_remove_events component_add_remove_events.cpp)
target_link_libraries(example_component_add_remove_events mustache)
