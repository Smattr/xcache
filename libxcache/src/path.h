#pragma once

#include "../../common/compiler.h"

/** make a path absolute
 *
 * \param cwd Current working directory
 * \param path An absolute or cwd-relative path
 * \return Absolute path equivalent or `NULL` on out-of-memory
 */
INTERNAL char *path_absolute(const char *cwd, const char *path);

/** join two path fragments together
 *
 * The `root` may or may not end in a '/'.
 *
 * \param root Path prefix
 * \param stem Path suffix
 * \return Joined path or `NULL` on out-of-memory
 */
INTERNAL char *path_join(const char *root, const char *stem);
