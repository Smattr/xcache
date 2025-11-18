#pragma once

#include "../../common/compiler.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern bool debug;
extern FILE *debug_file;

/// emit a debug message
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(debug)) {                                                     \
      const char *name_ = strrchr(__FILE__, '/');                              \
      FILE *const debug_file_ = debug_file == NULL ? stderr : debug_file;      \
      flockfile(debug_file_);                                                  \
      fprintf(debug_file_, "[XCACHE] xcache/src%s:%d: ", name_, __LINE__);     \
      fprintf(debug_file_, args);                                              \
      fprintf(debug_file_, "\n");                                              \
      funlockfile(debug_file_);                                                \
    }                                                                          \
  } while (0)
