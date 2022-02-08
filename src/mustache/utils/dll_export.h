#ifndef MUSTACHE_DLL_EXPORT_HPP
#define MUSTACHE_DLL_EXPORT_HPP

#ifdef _WIN32
    #ifdef MAKE_MUSTACHE_DLL
        #define MUSTACHE_EXPORT __declspec(dllexport)
    #else
        #define MUSTACHE_EXPORT __declspec(dllimport)
    #endif
    // TODO: fix this warning
    #pragma warning( disable : 4251 )
#else
    #define MUSTACHE_EXPORT
#endif

#endif //MUSTACHE_DLL_EXPORT_HPP
