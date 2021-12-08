#pragma once

#include <stddef.h>
#include <xcache/hash.h>

/// a file read by a process
typedef struct {
  /// absolute path to the file
  char *path;
  /// hash of the file’s content
  xc_hash_t hash;
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
