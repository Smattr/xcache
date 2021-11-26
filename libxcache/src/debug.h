#pragma once

#include "macros.h"
#include <stdio.h>

/// output stream for debugging information
extern FILE *debug;

/// write a debugging message
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(debug != NULL)) {                                             \
      fprintf(debug, "xcache: [DEBUG] ");                                      \
      fprintf(debug, args);                                                    \
      fprintf(debug, "\n");                                                    \
    }                                                                          \
  } while (0)
