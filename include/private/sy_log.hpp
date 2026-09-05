#pragma once

#include <OS/OSError.h>

#ifdef SY_DEBUG
#define SY_LOG(...) OSReport(__VA_ARGS__)
#else
#define SY_LOG(...) ((void)0)
#endif
