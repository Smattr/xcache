#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

/// a command to be executed
typedef struct {
  size_t argc; ///< number of command line arguments
  char **argv; ///< command line arguments
  char *cwd;   ///< directory the command starts from
} xc_cmd_t;

#ifdef __cplusplus
}
#endif
