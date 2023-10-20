#pragma once

#include "../../common/compiler.h"
#include "hash_t.h"
#include <stdbool.h>

/// a file/directory that was read
typedef struct {
  char *path;            ///< absolute path to the target
  bool exists : 1;       ///< did it exist on disk?
  bool accessible : 1;   ///< did we have permission to read it?
  bool is_directory : 1; ///< was it a directory (not a file)?
  hash_t digest;         ///< hash of the file’s contents
} input_t;

/** create a new input
 *
 * \param i [out] Created input on success
 * \param path Absolute path to the input
 * \return 0 on success or errno on failure
 */
INTERNAL int input_new(input_t *i, const char *path);

/** is this input still consistent with how it was originally perceived?
 *
 * \param i Input to examine
 */
INTERNAL bool input_is_valid(const input_t i);

/** destroy an input
 *
 * \param i Input whose backing memory to deallocate
 */
INTERNAL void input_free(input_t i);
