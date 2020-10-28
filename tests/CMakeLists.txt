cmake_minimum_required(VERSION 3.10)

project(mustache_test)
if (MSVC)
    find_package(GTest CONFIG REQUIRED)
else()
    include(GoogleTest)
    find_package(GTest REQUIRED)
endif(MSVC)

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
)

target_link_libraries(${PROJECT_NAME} PRIVATE mustache)

if (MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock)
else()
    gtest_add_tests(mustache_test "${FooTestArgs}" AUTO)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GTEST_BOTH_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GTEST_BOTH_LIBRARIES})
    if(MUSTACHE_TEST_SANITIZE)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
        set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
    endif()
endif(MSVC)
