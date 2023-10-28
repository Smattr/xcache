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

/** deallocate memory backing a command
 *
 * This assumes all `cmd` fields were heap-allocated. Do not call this function
 * if some `cmd` fields are static or stack-allocated.
 *
 * \param cmd Command to deallocate
 */
XCACHE_API void xc_cmd_free(xc_cmd_t cmd);

#ifdef __cplusplus
}
#endif
