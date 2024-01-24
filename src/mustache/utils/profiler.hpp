#pragma once


#ifdef MUSTACHE_BUILD_WITH_EASY_PROFILER
    #include <easy/profiler.h>

    #define MUSTACHE_PROFILER_BLOCK(...) EASY_BLOCK(__VA_ARGS__)
    #define MUSTACHE_PROFILER_START() EASY_PROFILER_ENABLE
    #define MUSTACHE_PROFILER_MAIN_THREAD() EASY_THREAD("MainThread")
    #define MUSTACHE_PROFILER_THREAD(name) EASY_THREAD(name)
    #define MUSTACHE_PROFILER_FRAME(...)
    #define MUSTACHE_PROFILER_DUMP(fileName) profiler::dumpBlocksToFile(fileName);
    #define MUSTACHE_PROFILER_CATEGORY(text, type) EASY_BLOCK(text)
#else
    #define MUSTACHE_PROFILER_BLOCK(...)
    #define MUSTACHE_PROFILER_START()
    #define MUSTACHE_PROFILER_MAIN_THREAD()
    #define MUSTACHE_PROFILER_THREAD(name)
    #define MUSTACHE_PROFILER_FRAME(...)
    #define MUSTACHE_PROFILER_DUMP(fileName)
    #define MUSTACHE_PROFILER_CATEGORY(text, type)
#endif

#ifndef MUSTACHE_PROFILER_LVL
    #define MUSTACHE_PROFILER_LVL 2
#endif

#if MUSTACHE_PROFILER_LVL >= 0
    // only heavy tasks like job start \ finish, world.update, etc.
    #define MUSTACHE_PROFILER_BLOCK_LVL_0(...) MUSTACHE_PROFILER_BLOCK(__VA_ARGS__)
#else
    #define MUSTACHE_PROFILER_BLOCK_LVL_0(...)
#endif

#if MUSTACHE_PROFILER_LVL >= 1
    //
    #define MUSTACHE_PROFILER_BLOCK_LVL_1(...) MUSTACHE_PROFILER_BLOCK(__VA_ARGS__)
#else
    #define MUSTACHE_PROFILER_BLOCK_LVL_1(...)
#endif

#if MUSTACHE_PROFILER_LVL >= 2
    //
    #define MUSTACHE_PROFILER_BLOCK_LVL_2(...) MUSTACHE_PROFILER_BLOCK(__VA_ARGS__)
#else
    #define MUSTACHE_PROFILER_BLOCK_LVL_2(...)
#endif

#if MUSTACHE_PROFILER_LVL >= 3
    // verbose mode
    #define MUSTACHE_PROFILER_BLOCK_LVL_3(...) MUSTACHE_PROFILER_BLOCK(__VA_ARGS__)
#else
    #define MUSTACHE_PROFILER_BLOCK_LVL_3(...)
#endif
