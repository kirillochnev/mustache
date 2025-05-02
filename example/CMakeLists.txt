# FIXME: avoid absolute paths and global includes, use target_include_directories
# TODO: add glm dependencies in cmake with FetchContent or use a dependency manager (conan/vcpkg)
include_directories("/home/kirill/Documents/projects/c++/ecs_bench/third_party/glm")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(OTHER_FLAGS "-s TOTAL_MEMORY=1024MB -s WASM=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OTHER_FLAGS}")
endif()

file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(example_source ${EXAMPLE_SOURCES})
    get_filename_component(example_filename ${example_source} NAME_WE)
    set(target_name "example_${example_filename}")

    add_executable(${target_name} ${example_source})
    target_link_libraries(${target_name} mustache)
    target_include_directories(${target_name} PRIVATE ${mustache_SOURCE_DIR}/src)

    set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
    )
endforeach()
