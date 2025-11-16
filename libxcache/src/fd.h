#pragma once

#include "../../common/compiler.h"
#include "list.h"
#include <stddef.h>

/// an open file descriptor in a subprocess
typedef struct {
  char *path;
} fd_t;

/// a file descriptor table
typedef struct {
  LIST(fd_t *) fds; ///< open file descriptors
  size_t ref_count; ///< number of threads using this table
} fds_t;

/// create a new file descriptor table
///
/// The reference count of the created file descriptor table is set to 1 on
/// success.
///
/// @return Created table on success or `NULL` on out-of-memory
INTERNAL fds_t *fds_new(void);

/// take a reference to a file descriptor table
///
/// This function increments the file descriptor table’s reference count and
/// returns the same pointer it was passed. It has this type signature to
/// encourage a universal reference-acquisition-and-assignment pattern:
///
///   my_thread->fd = fds_acquire(other_thread->fd);
///
/// @param fd Object to take a reference to
/// @return Same value as the input argument
INTERNAL fds_t *fds_acquire(fds_t *fd);

/// return a reference to a file descriptor table
///
/// This function decrements the file descriptor table’s reference count and
/// returns `NULL`. It has this type signature to encourage a universal
/// reference-release-and-null-out pattern:
///
///   my_thread->fd = fds_release(my_thread->fd);
///
/// If the reference count reaches 0 by calling this function, the file
/// descriptor table is also freed.
///
/// When passed `NULL`, this function is a no-op.
///
/// @param fd Object to return a reference to
/// @return `NULL`
INTERNAL fds_t *fds_release(fds_t *fd);

/// register a new open file descriptor
///
/// @param table File descriptor table to install into
/// @param fd File descriptor number
/// @param path Absolute path to the open file/directory
/// @return 0 on success or an errno on failure
INTERNAL int fd_open(fds_t *table, int fd, const char *path);

/// lookup a file descriptor
///
/// @param table File descriptor table to search
/// @param fd Number of the descriptor
/// @return The found descriptor or `NULL` if not found
INTERNAL fd_t *fd_at(fds_t *table, int fd);

/// deregister a file descriptor
///
/// @param table File descriptor table to search
/// @param fd File descriptor to deregister
INTERNAL void fd_close(fds_t *table, int fd);
