#ifndef UTILS_H
#define UTILS_H

#include <assert.h>

#define force_inline __forceinline
#define StaticAssert(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]
#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif
