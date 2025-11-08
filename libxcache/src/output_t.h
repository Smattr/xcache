#pragma once

#include "../../common/compiler.h"
#include "list.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
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

/// create a new chmod output
///
/// @param output [out] Created output on success
/// @param path Path being chmod-ed
/// @param mode Mode being set
/// @return 0 on success or an errno on failure
INTERNAL int output_new_chmod(output_t *output, const char *path, mode_t mode);

/// create a new write output
///
/// This does not populate the `cached_copy` member, assuming it will be
/// populated later.
///
/// @param output [out] Created output on success
/// @param path Path being written
/// @return 0 on success or an errno on failure
INTERNAL int output_new_write(output_t *output, const char *path);

/// deserialise an output from a file
///
/// @param output [out] Reconstructed output on success
/// @param stream File to read from
/// @return 0 on success or an errno on failure
INTERNAL int output_load(output_t *output, FILE *stream);

/// serialise an output to a file
///
/// @param output Output to write out
/// @param stream File to write to
/// @return 0 on success or an errno on failure
INTERNAL int output_save(const output_t output, FILE *stream);

/// re-run the effects of an output
///
/// @param output Output to replay
/// @param owner Trace within which this output lives
/// @return 0 on success or an errno on failure
INTERNAL int output_replay(const output_t output, const xc_trace_t *owner);

/// compare two outputs for equality
///
/// For `OUT_WRITE` outputs, this ignores the `cached_copy` member that may not
/// be populated or may point to disparate data.
///
/// @param a First operand to the comparison
/// @param b Second operand to the comparison
/// @return True if equal
INTERNAL bool output_eq(const output_t a, const output_t b);

/// copy an output
///
/// @param dst Copied output on success
/// @param src Output to copy
/// @return 0 on success or an errno on failure
INTERNAL int output_dup(output_t *dst, const output_t src);

/// destroy an output
///
/// @param output Output whose backing memory to deallocate
INTERNAL void output_free(output_t output);

/// a collection of outputs
typedef LIST(output_t) outputs_t;
