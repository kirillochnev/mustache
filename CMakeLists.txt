cmake_minimum_required(VERSION 3.7)

project(mustache
    VERSION 0.2
    LANGUAGES C CXX
    HOMEPAGE_URL https://github.com/kirillochnev/mustache
)

message(VERBOSE "*")
message(VERBOSE "* ${PROJECT_NAME} v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message(VERBOSE "* Copyright (c) 2020-2022 Kirill Ochnev <kirillochnev@gmail.com>")
message(VERBOSE "*")

set(MUSTACHE_VERSION ${PROJECT_VERSION})

find_package(Threads REQUIRED)

set(MUSTACHE_CXX_STD cxx_std_17 CACHE STRING "C++ standard revision to build mustache")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(MUSTACHE_USE_SANITIZER "Enable sanitizers by adding -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined flags if availbale." OFF)
option(MUSTACHE_WARNINGS_AS_ERROR "Enable -Werror and some of extra warnings" ON)
option(MUSTACHE_BUILD_SHARED "Build shared libraries?" ON)

if(MUSTACHE_USE_SANITIZER)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

include(GNUInstallDirs)

if (MUSTACHE_BUILD_SHARED)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(MUSTACHE_LIB_TYPE SHARED)
else()
    set(MUSTACHE_LIB_TYPE STATIC)
endif ()

add_library(${PROJECT_NAME} ${MUSTACHE_LIB_TYPE}
    ${mustache_SOURCE_DIR}/src/mustache/utils/dll_export.h
    ${mustache_SOURCE_DIR}/src/mustache/utils/timer.cpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/timer.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/logger.cpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/logger.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/type_info.cpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/type_info.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/benchmark.cpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/benchmark.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/uncopiable.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/memory_manager.cpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/memory_manager.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/dispatch.cpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/dispatch.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/index_like.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/array_wrapper.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/invoke.hpp
    ${mustache_SOURCE_DIR}/src/mustache/utils/crc32.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/entity.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/entity.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/archetype_operation_helper.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/archetype_operation_helper.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/temporal_storage.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/temporal_storage.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/entity_manager.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/entity_manager.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/base_job.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/base_job.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/job.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/non_template_job.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/non_template_job.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/system.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/system.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/system_manager.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/system_manager.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/archetype.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/archetype.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/job_arg_parcer.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_handler.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_mask.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_mask.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/world.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/world.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_info.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_info.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_factory.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_factory.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/entity_group.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/entity_group.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/world_filter.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/world_filter.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/base_component_data_storage.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/base_component_data_storage.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/default_component_data_storage.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/default_component_data_storage.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/new_component_data_storage.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/new_component_data_storage.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/event_manager.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/event_manager.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/entity_builder.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/entity_builder.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/world_storage.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/world_storage.hpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_version_storage.cpp
    ${mustache_SOURCE_DIR}/src/mustache/ecs/component_version_storage.hpp
    ${mustache_SOURCE_DIR}/src/mustache/c_api.cpp
    ${mustache_SOURCE_DIR}/src/mustache/c_api.h
)
if (MUSTACHE_WARNINGS_AS_ERROR)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        include(cmake/gcc_warnings.cmake)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        include(cmake/clang_warnings.cmake)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        include(cmake/msvc_warnings.cmake)
    endif()
    
    set_property(TARGET ${PROJECT_NAME} PROPERTY COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${MUSTACHE_WARNINGS}")
else()
    message(WARNING "If you have disabled compiler warnings because your compiler throws errors, please let me know.")
endif()

add_subdirectory(third_party)
target_link_libraries(${PROJECT_NAME} mustache_profiler)


include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})
if (MUSTACHE_BUILD_SHARED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE mustache_EXPORTS)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${mustache_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${PROJECT_NAME} PUBLIC ${MUSTACHE_CXX_STD})

option(MUSTACHE_BUILD_TESTS "Build mustache Tests" OFF)
if (MUSTACHE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(MUSTACHE_BUILD_EXAMPLES "Build mustache Examples" OFF)
if (MUSTACHE_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

set(mustache_libs -lmustache)

configure_file(
        ${mustache_SOURCE_DIR}/mustache.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/mustache.pc
)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/mustache.pc
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(
        DIRECTORY src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
)
# TODO: add mustache_export.h to install files
#install(
#        FILES ${PROJECT_BINARY_DIR}/mustache_export.h 
#        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#)

install(
        DIRECTORY src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
        TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "bin"
)