#pragma once

#include <assert.h>

#define INTERNAL __attribute__((visibility("internal")))

#ifdef __GNUC__
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#define UNLIKELY(expr) __builtin_expect((expr), 0)
#else
#define LIKELY(expr) (expr)
#define UNLIKELY(expr) (expr)
#endif

#ifdef __GNUC__
#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert("unreachable" && 0);                                                \
    __builtin_unreachable();                                                   \
  } while (0)
#else
#define UNREACHABLE() assert("unreachable" && 0);
#endif
