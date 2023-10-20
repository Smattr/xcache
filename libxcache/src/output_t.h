#pragma once

#include "../../common/compiler.h"
#include <sys/stat.h>
#include <xcache/trace.h>

typedef struct {
  char *path;        ///< absolute path to target
  mode_t st_mode;    ///< chmod-ed mode to set
  uid_t st_uid;      ///< chown-ed uid to set
  gid_t st_gid;      ///< chown-ed gid to set
  char *cached_copy; ///< related path to cached content, if not directory
} output_t;

INTERNAL int output_replay(const output_t output, const xc_trace_t *owner);

/** destroy an output
 *
 * \param output Output whose backing memory to deallocate
 */
INTERNAL void output_free(output_t output);
