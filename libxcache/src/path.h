#pragma once

#include "../../common/compiler.h"
#include <stdbool.h>

/// make a path absolute
///
/// @param cwd Current working directory
/// @param path An absolute or cwd-relative path
/// @return Absolute path equivalent or `NULL` on out-of-memory
INTERNAL char *path_absolute(const char *cwd, const char *path);

/// join two path fragments together
///
/// The `root` may or may not end in a '/'.
///
/// @param root Path prefix
/// @param stem Path suffix
/// @return Joined path or `NULL` on out-of-memory
INTERNAL char *path_join(const char *root, const char *stem);

/// create a new unique file
///
/// @param root Directory in which to create the file
/// @param suffix Optional suffix to give the new filename
/// @param fd [out] RW file descriptor to the file on success
/// @param path [out] If not `NULL`, absolute path to the file on success
/// @return 0 on success or an errno on failure
INTERNAL int path_make(const char *root, const char *suffix, int *fd,
                       char **path);

/// is this a file whose reads and writes we can cache?
///
/// @param abs_path Absolute path to file to consider
/// @return True if I/O to the file can be cached
INTERNAL bool path_is_cacheable(const char *abs_path);
