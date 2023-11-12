#pragma once

#include "../../common/compiler.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern FILE *xc_debug INTERNAL;

/// emit a debug message
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(xc_debug != NULL)) {                                          \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(xc_debug);                                                     \
      fprintf(xc_debug, "[XCACHE] libxcache/src%s:%d: ", name_, __LINE__);     \
      fprintf(xc_debug, args);                                                 \
      fprintf(xc_debug, "\n");                                                 \
      funlockfile(xc_debug);                                                   \
    }                                                                          \
  } while (0)

/// logging wrapper for error conditions
#define ERROR(cond)                                                            \
  ({                                                                           \
    bool cond_ = (cond);                                                       \
    if (UNLIKELY(cond_)) {                                                     \
      int errno_ = errno;                                                      \
      DEBUG("`%s` failed (current errno is %d)", #cond, errno_);               \
      errno = errno_;                                                          \
    }                                                                          \
    cond_;                                                                     \
  })

/** convert the file descriptor from an openat() call to a string
 *
 * This function heap-allocates. Do not call it on a hot path.
 *
 * \param fd Descriptor to translate
 * \return A string representation or `NULL` on out-of-memory
 */
INTERNAL char *atfd_to_str(long fd);

/** convert flags from an open() call to a string
 *
 * This function heap-allocates. Do not call it on a hot path.
 *
 * \param flags Flags to translate
 * \return A string representation or `NULL` on out-of-memory
 */
INTERNAL char *openflags_to_str(long flags);

/** convert flags from a newfstatat() call to a string
 *
 * This function heap-allocates. Do not call it on a hot path.
 *
 * \param flags Flags to translate
 * \return A string representation or `NULL` on out-of-memory
 */
INTERNAL char *statflags_to_str(long flags);
