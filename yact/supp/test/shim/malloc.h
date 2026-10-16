// Minimal <malloc.h> for host builds (macOS has no malloc.h): the sources only
// use the standard allocation functions.
#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define _alloca(x) __builtin_alloca(x)
#endif

#ifdef __cplusplus
}
#endif
