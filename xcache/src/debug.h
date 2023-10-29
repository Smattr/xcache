#pragma once

#include "../../common/compiler.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern bool debug;

/// emit a debug message
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(debug)) {                                                     \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(stderr);                                                       \
      fprintf(stderr, "[XCACHE] xcache/src%s:%d: ", name_, __LINE__);          \
      fprintf(stderr, args);                                                   \
      fprintf(stderr, "\n");                                                   \
      funlockfile(stderr);                                                     \
    }                                                                          \
  } while (0)
