#pragma once

#include "macros.h"
#include <stddef.h>
#include <xcache/hash.h>
#include <xcache/trace.h>

/// a file read by a process
typedef struct {
  /// absolute path to the file
  char *path;
  /// hash of the file’s content
  xc_hash_t hash;
  /// TODO: capture ENOENT and EPERM/EACCES
} read_file;

/// a file written by a process
typedef struct {
  /// absolute path to the file
  char *path;
  /// path to a file containing the content written
  char *content_path;
} written_file;

struct xc_trace {

  /// files read
  size_t read_len;
  read_file *read;

  /// files written
  size_t written_len;
  written_file *written;

  /// status the process returned on exit
  int exit_status;
};

/// append an entry to the read files list
///
/// \param trace Trace to operate on
/// \param f Entry to append
/// \return 0 on success or an errno on failure
INTERNAL int trace_append_read(xc_trace_t *trace, read_file f);

/// clean up and deallocate the members of a trace
INTERNAL void trace_deinit(xc_trace_t *trace);
