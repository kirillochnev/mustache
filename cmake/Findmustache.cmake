# Look for a version of mustache on the local machine
#
# By default, this will look in all common places. If mustache is built or
# installed in a custom location, you're able to either modify the
# CMakeCache.txt file yourself or simply pass the path to CMake using either the
# environment variable `MUSTACHE_ROOT` or the CMake define with the same name.

set(MUSTACHE_PATHS
        ${MUSTACHE_ROOT}
        $ENV{MUSTACHE_ROOT}
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt
        )

find_path(MUSTACHE_INCLUDE_DIR mustache/ecs/ecs.hpp PATH_SUFFIXES include PATHS ${MUSTACHE_PATHS})
find_library(MUSTACHE_LIBRARY NAMES mustache PATH_SUFFIXES lib PATHS ${MUSTACHE_PATHS})
find_library(MUSTACHE_LIBRARY_DEBUG NAMES mustache-d PATH_SUFFIXES lib PATHS ${MUSTACHE_PATHS})

set(MUSTACHE_LIBRARIES ${MUSTACHE_LIBRARY})
set(MUSTACHE_INCLUDE_DIRS ${MUSTACHE_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set MUSTACHE__FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(mustache DEFAULT_MSG MUSTACHE_LIBRARY MUSTACHE_INCLUDE_DIR)

mark_as_advanced(MUSTACHE_INCLUDE_DIR MUSTACHE_LIBRARY)
