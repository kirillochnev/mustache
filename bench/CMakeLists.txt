include_directories("/home/kirill/Documents/projects/c++/ecs_bench/third_party/glm")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(OTHER_FLAGS "-s TOTAL_MEMORY=1024MB -s WASM=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OTHER_FLAGS}")
endif()

add_executable(mustache_exe
        main.cpp
        component_data_storage_bench.cpp
)

target_link_libraries(mustache_exe mustache)

