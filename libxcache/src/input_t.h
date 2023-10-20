#pragma once

#include "../../common/compiler.h"
#include "hash_t.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

/// a file/directory that was read
typedef struct {
  char *path;     ///< absolute path to the target
  int stat_errno; ///< errno from `stat(path)`
  mode_t st_mode; ///< stat-ed mode
  uid_t st_uid;   ///< stat-ed uid
  gid_t st_gid;   ///< stat-ed gid
  size_t st_size; ///< on-disk size
  int open_errno; ///< errno from `open(path, …)`
  hash_t digest;  ///< hash of the file’s contents
} input_t;

/** create a new input
 *
 * \param input [out] Created input on success
 * \param path Absolute path to the input
 * \return 0 on success or errno on failure
 */
INTERNAL int input_new(input_t *input, const char *path);

/** deserialise an input from a file
 *
 * \param input [out] Reconstructed input on success
 * \param stream File to read from
 * \return 0 on success or an errno on failure
 */
INTERNAL int input_read(input_t *input, FILE *stream);

/** is this input still consistent with how it was originally perceived?
 *
 * \param input Input to examine
 */
INTERNAL bool input_is_valid(const input_t input);

/** destroy an input
 *
 * \param i Input whose backing memory to deallocate
 */
INTERNAL void input_free(input_t i);
