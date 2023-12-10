#pragma once

#include "../../common/compiler.h"
#include <stdio.h>
#include <sys/stat.h>
#include <xcache/trace.h>

typedef enum {
  OUT_CHMOD, ///< chmod()
  OUT_CHOWN, ///< chown()
  OUT_MKDIR, ///< mkdir()
  OUT_WRITE, ///< open() with O_WRONLY or O_RDWR
} output_type_t;

typedef struct {
  output_type_t tag; ///< discriminator of the union
  char *path;        ///< absolute path to target
  union {
    struct {
      mode_t mode; ///< chmod-ed mode to set
    } chmod;
    struct {
      uid_t uid; ///< chown-ed uid to set
      gid_t gid; ///< chown-ed gid to set
    } chown;
    struct {
      mode_t mode; ///< mkdir mode to set
    } mkdir;
    struct {
      char *cached_copy; ///< relative path to cached content
    } write;
  };
} output_t;

/** deserialise an output from a file
 *
 * \param output [out] Reconstructed output on success
 * \param stream File to read from
 * \return 0 on success or an errno on failure
 */
INTERNAL int output_load(output_t *output, FILE *stream);

/** serialise an output to a file
 *
 * \param output Output to write out
 * \param stream File to write to
 * \return 0 on success or an errno on failure
 */
INTERNAL int output_save(const output_t output, FILE *stream);

/** re-run the effects of an output
 *
 * \param output Output to replay
 * \param owner Trace within which this output lives
 * \return 0 on success or an errno on failure
 */
INTERNAL int output_replay(const output_t output, const xc_trace_t *owner);

/** copy an output
 *
 * \param dst Copied output on success
 * \param src Output to copy
 * \return 0 on success or an errno on failure
 */
INTERNAL int output_dup(output_t *dst, const output_t src);

/** destroy an output
 *
 * \param output Output whose backing memory to deallocate
 */
INTERNAL void output_free(output_t output);
