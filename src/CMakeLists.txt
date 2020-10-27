cmake_minimum_required(VERSION 3.10)

project(mustache VERSION 0.1 LANGUAGES CXX)


find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} STATIC
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
        mustache/utils/index_like.hpp
        mustache/utils/array_wrapper.hpp

        mustache/ecs/entity.cpp
        mustache/ecs/entity.hpp
        mustache/ecs/archetype_operation_helper.cpp
        mustache/ecs/archetype_operation_helper.hpp
        mustache/ecs/entity_manager.cpp
        mustache/ecs/entity_manager.hpp
        mustache/ecs/entity_manager.hpp
        mustache/ecs/base_job.cpp
        mustache/ecs/base_job.hpp
        mustache/ecs/job.hpp
        mustache/ecs/system.cpp
        mustache/ecs/system.hpp
        mustache/ecs/system_manager.cpp
        mustache/ecs/system_manager.hpp
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
        mustache/ecs/entity_group.cpp
        mustache/ecs/entity_group.hpp
        mustache/ecs/world_filter.cpp
        mustache/ecs/world_filter.hpp
        mustache/ecs/component_data_storage.cpp
        mustache/ecs/component_data_storage.hpp
        mustache/ecs/new_component_data_storage.cpp
        mustache/ecs/new_component_data_storage.hpp
)

target_include_directories(${PROJECT_NAME}
        PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
