#pragma once

#include "../../common/compiler.h"

/** join two path fragments together
 *
 * The `root` may or may not end in a '/'.
 *
 * \param root Path prefix
 * \param stem Path suffix
 * \return Joined path or `NULL` on out-of-memory
 */
INTERNAL char *path_join(const char *root, const char *stem);
