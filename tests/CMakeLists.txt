cmake_minimum_required(VERSION 3.7)


project(mustache_test)

enable_testing()

if (MSVC)
    find_package(GTest CONFIG)
    set(gtest_libs GTest::gtest GTest::gtest_main)
else()
	find_package(GTest REQUIRED)
	set(gtest_libs ${GTEST_BOTH_LIBRARIES})
endif(MSVC)

# fallback for gtest not found
if (NOT GoogleTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        release-1.12.1
    )
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    set(BUILD_GTEST OFF CACHE BOOL "" FORCE)
    if(MSVC)
    set(gtest_force_shared_crt ON)
    endif()
    FetchContent_MakeAvailable(googletest)
    set(gtest_libs gtest gmock gtest_main)
endif()

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME}
        main.cpp
        type_info.cpp
        entity_manager.cpp
        job.cpp
        entity.cpp
        component_factory.cpp
        world_filter.cpp
        component_mask_test.cpp
        dispatcher.cpp
        system.cpp
        event_manager.cpp
        world_storage.cpp
        shared_component.cpp
        mutate_while_iteration.cpp
        c_api.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE mustache)
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

if(MUSTACHE_TEST_SANITIZE)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()


target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ${GTEST_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${gtest_libs})
