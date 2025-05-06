cmake_minimum_required(VERSION 3.14)
project(mustache_profiler
        VERSION ${PROJECT_VERSION}
        LANGUAGES CXX
)

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 0.2)
endif()

add_library(mustache_profiler INTERFACE)
add_library(mustache::profiler ALIAS mustache_profiler)

option(MUSTACHE_BUILD_WITH_EASY_PROFILER "Use easy profiler" OFF)
set(MUSTACHE_PROFILER_LVL "0" CACHE STRING "Set profiler level (0 = off, 1 = basic(recommended), 2 = detailed, 3 = deep)")

math(EXPR MUSTACHE_PROFILER_LVL_INT "${MUSTACHE_PROFILER_LVL}")
if(MUSTACHE_PROFILER_LVL_INT GREATER 0)
    set(MUSTACHE_BUILD_WITH_EASY_PROFILER ON CACHE BOOL "Use easy profiler" FORCE)
endif()


if(MUSTACHE_BUILD_WITH_EASY_PROFILER)
    target_compile_definitions(mustache_profiler INTERFACE MUSTACHE_BUILD_WITH_EASY_PROFILER=1)
    if(MUSTACHE_PROFILER_LVL_INT GREATER 0)
        target_compile_definitions(mustache_profiler INTERFACE MUSTACHE_PROFILER_LVL=${MUSTACHE_PROFILER_LVL})
    endif()

    include(FetchContent)

    set(EASY_PROFILER_NO_GUI ON CACHE BOOL "Disable GUI components" FORCE)
    set(EASY_PROFILER_NO_SAMPLES ON CACHE BOOL "Disable samples" FORCE)

    FetchContent_Declare(
            easy_profiler
            GIT_REPOSITORY https://github.com/yse/easy_profiler.git
            GIT_TAG        v2.1.0
    )

    FetchContent_MakeAvailable(easy_profiler)

    target_link_libraries(mustache_profiler INTERFACE easy_profiler)

    install(TARGETS easy_profiler EXPORT mustache_profilerTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(TARGETS mustache_profiler EXPORT mustache_profilerTargets)
install(EXPORT mustache_profilerTargets
        FILE mustache_profilerTargets.cmake
        NAMESPACE mustache::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mustache_profiler
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/mustache_profilerConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/mustache_profilerConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mustache_profiler
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/mustache_profilerConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mustache_profilerConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/mustache_profilerConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mustache_profiler
)