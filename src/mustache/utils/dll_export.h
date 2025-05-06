#pragma once

#include "mustache_export.h"

// Disable warning C4251 only for old MSVC (before VS2019 16.3)
#if defined(_MSC_VER) && (_MSC_VER < 1923)
    #pragma warning(disable : 4251)
#endif
