#pragma once

#include "macros.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/// output stream for debugging information
extern FILE *debug;

/// write a debugging message
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(debug != NULL)) {                                             \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(debug);                                                        \
      fprintf(debug, "xcache:libxcache/src%s:%d: [DEBUG] ", name_, __LINE__);  \
      fprintf(debug, args);                                                    \
      fprintf(debug, "\n");                                                    \
      funlockfile(debug);                                                      \
    }                                                                          \
  } while (0)

/// logging wrapper for error conditions
#define ERROR(cond)                                                            \
  ({                                                                           \
    bool cond_ = (cond);                                                       \
    if (UNLIKELY(cond_)) {                                                     \
      int errno_ = errno;                                                      \
      DEBUG("`%s` failed", #cond);                                             \
      errno = errno_;                                                          \
    }                                                                          \
    cond_;                                                                     \
  })
