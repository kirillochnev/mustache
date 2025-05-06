cmake_minimum_required(VERSION 3.14)
project(mustache
        VERSION 0.2
        LANGUAGES CXX
        HOMEPAGE_URL "https://github.com/kirillochnev/mustache"
)

# Basic configuration
message(VERBOSE "*")
message(VERBOSE "* ${PROJECT_NAME} v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message(VERBOSE "* Copyright (c) 2020-2025 Kirill Ochnev <kirillochnev@gmail.com>")
message(VERBOSE "*")

set(MUSTACHE_VERSION ${PROJECT_VERSION})

# Options
option(MUSTACHE_BUILD_SHARED "Build as shared library" ON)
option(MUSTACHE_WARNINGS_AS_ERROR "Treat warnings as errors" ON)
option(MUSTACHE_USE_SANITIZER "Enable address sanitizer" OFF)
option(MUSTACHE_BUILD_WITH_EASY_PROFILER "Enable easy_profiler integration" OFF)
option(MUSTACHE_BUILD_TESTS "Build tests" OFF)
option(MUSTACHE_BUILD_EXAMPLES "Build examples" OFF)

# Dependencies
find_package(Threads REQUIRED)

# Global settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MUSTACHE_USE_SANITIZER)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fno-omit-frame-pointer)
endif()

include(GNUInstallDirs)
include(FetchContent)
include(GenerateExportHeader)

# Add profiler subdirectory
add_subdirectory(third_party)

# Collect sources while preserving directory structure
file(GLOB_RECURSE MUSTACHE_SOURCES CONFIGURE_DEPENDS src/mustache/*.cpp)
file(GLOB_RECURSE MUSTACHE_HEADERS CONFIGURE_DEPENDS
        src/mustache/*.hpp
        src/mustache/*.h
        src/mustache/ecs/*.hpp
        src/mustache/ecs/*.h
        src/mustache/ext/*.hpp
        src/mustache/utils/*.hpp
        src/mustache/utils/*.h
)

# Create library
if(MUSTACHE_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${MUSTACHE_SOURCES} ${MUSTACHE_HEADERS})
else ()
    add_library(${PROJECT_NAME} STATIC ${MUSTACHE_SOURCES} ${MUSTACHE_HEADERS})
endif ()
# Generate export header in correct subdirectory
set(EXPORT_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${EXPORT_HEADER_DIR})

# Generate export header
generate_export_header(${PROJECT_NAME}
        BASE_NAME mustache
        EXPORT_MACRO_NAME MUSTACHE_EXPORT
        EXPORT_FILE_NAME mustache_export.h
        STATIC_DEFINE MUSTACHE_STATIC_DEFINE
)

# Library properties
set_target_properties(${PROJECT_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 0
)

if(MUSTACHE_BUILD_SHARED)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC MUSTACHE_STATIC)
endif()

# Compiler warnings
if (MUSTACHE_WARNINGS_AS_ERROR)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        include(cmake/gcc_warnings.cmake)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        include(cmake/clang_warnings.cmake)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        include(cmake/msvc_warnings.cmake)
    endif()

    target_compile_options(${PROJECT_NAME} PRIVATE ${MUSTACHE_WARNINGS})
else()
    message(WARNING "If you have disabled compiler warnings because your compiler throws errors, please let me know.")
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
        Threads::Threads
        mustache::profiler
)

# Installation - preserve directory structure
install(
        DIRECTORY src/mustache/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mustache
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.inl"
        PATTERN "*.cpp" EXCLUDE
)

install(
        FILES ${EXPORT_HEADER_DIR}/mustache_export.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mustache/utils
)

install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Config files
configure_file(mustache.pc.in mustache.pc @ONLY)
install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/mustache.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# CMake package config
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        cmake/mustacheConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Tests and examples
if(MUSTACHE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MUSTACHE_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()