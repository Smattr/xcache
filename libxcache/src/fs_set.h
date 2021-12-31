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
/// If `content_path` is non-null, it will be assumed to be the final location
/// of cached data for this written file. That is, this file does not need to be
/// fully written (or even exist) at the time the `fs_set_add_write` call is
/// made. If it is null, it is assumed the caller will later update the added
/// entry’s `content_path` member.
///
/// \param set Set to add to
/// \param path Path to mark as written
/// \param content_path Optional path to where a cached copy of this file was
///   stored
/// \return 0 on success or an errno on failure
INTERNAL int fs_set_add_write(fs_set_t *set, const char *path,
                              const char *content_path);

/// clear a set and deallocate any contained entries
///
/// \param set Set to operate on
INTERNAL void fs_set_deinit(fs_set_t *set);
