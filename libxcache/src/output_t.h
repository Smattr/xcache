#pragma once

#include "../../common/compiler.h"
#include <stdio.h>
#include <sys/stat.h>
#include <xcache/trace.h>

typedef struct {
  char *path;        ///< absolute path to target
  mode_t mode;       ///< chmod-ed mode to set
  uid_t uid;         ///< chown-ed uid to set
  gid_t gid;         ///< chown-ed gid to set
  char *cached_copy; ///< relative path to cached content, if not directory
} output_t;

/** deserialise an output from a file
 *
 * \param output [out] Reconstructed output on success
 * \param stream File to read from
 * \return 0 on success or an errno on failure
 */
INTERNAL int output_load(output_t *output, FILE *stream);

/** re-run the effects of an output
 *
 * \param output Output to replay
 * \param owner Trace within which this output lives
 * \return 0 on success or an errno on failure
 */
INTERNAL int output_replay(const output_t output, const xc_trace_t *owner);

/** destroy an output
 *
 * \param output Output whose backing memory to deallocate
 */
INTERNAL void output_free(output_t output);
