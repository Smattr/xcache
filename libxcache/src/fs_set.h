#pragma once

#include "macros.h"
#include <stddef.h>
#include <xcache/hash.h>

/// a file or directory that was accessed by a tracee
typedef struct {

  /// absolute path to the file or directory
  char *path;

  /// Was this file read? This is only set if the file was not written _prior_
  /// to being read.
  unsigned read : 1;

  /// if `read`, did this file exist?
  unsigned existed : 1;

  /// if `read`, did the tracee (and the tracer) have permission to read it?
  unsigned accessible : 1;

  /// was this file written?
  unsigned written : 1;

  /// The digest of this file. Valid iff `read && existed && accessible`.
  xc_hash_t hash;

  /// Path to a file containing a copy of the written content. Valid and
  /// non-null iff `written`.
  char *content_path;

} fs_t;

/// a set of `fs_t` objects
typedef struct {
  fs_t *base;
  size_t size;
  size_t capacity;
} fs_set_t;

/// add a file or directory that is being read
///
/// \param set Set to add to
/// \param path Path to mark as read
/// \return 0 on success or an errno on failure
INTERNAL int fs_set_add_read(fs_set_t *set, const char *path);

/// add a file or directory that is being written
///
/// \param set Set to add to
/// \param path Path to mark as written
/// \return 0 on success or an errno on failure
INTERNAL int fs_set_add_write(fs_set_t *set, const char *path);

/// clear a set and deallocate any contained entries
///
/// \param set Set to operate on
INTERNAL void fs_set_free(fs_set_t *set);
