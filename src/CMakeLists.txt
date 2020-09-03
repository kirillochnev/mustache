cmake_minimum_required(VERSION 3.10)

project(mustache VERSION 0.1 LANGUAGES CXX)


find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} main.cpp
        mustache/utils/timer.cpp
        mustache/utils/timer.hpp
        mustache/utils/logger.cpp
        mustache/utils/logger.hpp
        mustache/utils/type_info.cpp
        mustache/utils/type_info.hpp
        mustache/utils/benchmark.cpp
        mustache/utils/benchmark.hpp
        mustache/utils/uncopiable.hpp
        mustache/utils/memory_manager.cpp
        mustache/utils/memory_manager.hpp
        mustache/utils/dispatch.cpp
        mustache/utils/dispatch.hpp


        mustache/ecs/entity.cpp
        mustache/ecs/entity.hpp
        mustache/ecs/archetype_operation_helper.cpp
        mustache/ecs/archetype_operation_helper.hpp
        mustache/ecs/entity_manager.cpp
        mustache/ecs/entity_manager.hpp
        mustache/ecs/entity_manager.hpp
        mustache/ecs/job.cpp
        mustache/ecs/job.hpp
        mustache/ecs/system.cpp
        mustache/ecs/system.hpp
        mustache/ecs/archetype.cpp
        mustache/ecs/archetype.hpp
        mustache/ecs/job_arg_parcer.hpp
        mustache/ecs/component_handler.hpp
        mustache/ecs/component_mask.cpp
        mustache/ecs/component_mask.hpp
        mustache/ecs/world.cpp
        mustache/ecs/world.hpp
        mustache/ecs/component_factory.cpp
        mustache/ecs/component_factory.hpp
        mustache/ecs/chunk.cpp
        mustache/ecs/chunk.hpp
        mustache/ecs/entity_group.cpp
        mustache/ecs/entity_group.hpp
)

target_include_directories(${PROJECT_NAME}
        PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()