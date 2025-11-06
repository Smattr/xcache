#pragma once

#include <stdbool.h>
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

/// create a command
///
/// `cwd` can be given as `NULL` to indicate the current directory of the
/// caller.
///
/// @param cmd [out] Created command on success
/// @param argc Number of command line arguments
/// @param argv Command line arguments
/// @param cwd Current working directory
/// @return 0 on success or an errno on failure
XCACHE_API int xc_cmd_new(xc_cmd_t *cmd, size_t argc, char **argv,
                          const char *cwd);

/// compare two commands for equality
///
/// @param a First operand
/// @param b Second operand
/// @return True if the commands are equal
XCACHE_API bool xc_cmd_eq(const xc_cmd_t a, const xc_cmd_t b);

/// execute a command
///
/// This is a thin wrapper around `execvp`. On success, this function does not
/// return.
///
/// @param cmd Command to execute
/// @return an errno on failure
XCACHE_API int xc_cmd_exec(const xc_cmd_t cmd);

/// deallocate memory backing a command
///
/// This assumes all `cmd` fields were heap-allocated. Do not call this function
/// if some `cmd` fields are static or stack-allocated.
///
/// @param cmd Command to deallocate
XCACHE_API void xc_cmd_free(xc_cmd_t cmd);

#ifdef __cplusplus
}
#endif
