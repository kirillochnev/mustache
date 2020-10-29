cmake_minimum_required(VERSION 3.10)
project("")
set(CMAKE_CXX_STANDARD 17)

# TODO: Use list
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    include(cmake/gcc_warnings.cmake)

    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    include(cmake/clang_warnings.cmake)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    include(cmake/msvc_warnings.cmake)
endif()

if(MUSTACHE_SANITIZE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

add_subdirectory(src)
if (MUSTACHE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (MUSTACHE_BUILD_BENCHMARK)
    add_subdirectory(bench)
endif()
