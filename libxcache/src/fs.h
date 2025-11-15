/// @file
/// @brief Abstraction over filesystem information associated with a thread

#pragma once

#include "../../common/compiler.h"
#include <stddef.h>

/// filesystem information for a subprocess
typedef struct {
  char *cwd; ///< current working directory

  /// number of threads using this filesystem information
  size_t ref_count;
} fs_t;

/// create a new filesystem information object
///
/// @param cwd Initial current working directory
/// @return A created object on success or `NULL` on out-of-memory
INTERNAL fs_t *fs_new(const char *cwd);

/// take a reference to a filesystem information object
///
/// This function increments the filesystem information’s reference count and
/// returns the same pointer it was passed. It has this type signature to
/// encourage a universal reference-acquisition-and-assignment pattern:
///
///   my_thread->fs = fs_acquire(other_thread->fs);
///
/// @param fs Object to take a reference to
/// @return Same value as the input argument
INTERNAL fs_t *fs_acquire(fs_t *fs);

/// return a reference to a filesystem information object
///
/// This function decrements the filesystem information object’s reference count
/// and returns `NULL`. It has this type signature to encourage a universal
/// reference-release-and-null-out pattern:
///
///   my_thread->fs = fs_release(my_thread->fs);
///
/// If the reference count reaches 0 by calling this function, the filesystem
/// object is also freed.
///
/// @param fs Object to return a reference to
/// @return `NULL`
INTERNAL fs_t *fs_release(fs_t *fs);

/// change current working directory
///
/// @param fs Filesystem information object to operate on
/// @param cwd New current working directory
/// @return 0 on success or an errno on failure
INTERNAL int fs_chdir(fs_t *fs, const char *cwd);
