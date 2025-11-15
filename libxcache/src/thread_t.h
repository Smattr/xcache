#pragma once

#include "../../common/compiler.h"
#include "fs.h"
#include "list.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <xcache/cmd.h>
#include <xcache/record.h>

/// an open file descriptor in a subprocess
typedef struct {
  char *path;
} fd_t;

/// create a file descriptor
///
/// @param fd [out] Created file descriptor on success
/// @param path Absolute path of the target file/directory
/// @return 0 on success or an errno on failure
INTERNAL int fd_new(fd_t **fd, const char *path);

/// destroy a file descriptor
///
/// @param fd File descriptor to deallocate
INTERNAL void fd_free(fd_t *fd);

/// a process being traced
typedef struct {
  pid_t id; ///< process identifier

  LIST(fd_t *) fds; ///< file descriptor table

  /// number of threads homed within this process
  ///
  /// The expectation is that this dropping to 0 represents the termination of
  /// the process.
  size_t reference_count;
} proc_t;

/// a thread within a process
typedef struct {
  pid_t id;                 ///< thread identifier
  proc_t *proc;             ///< containing process
  fs_t *fs;                 ///< filesystem
  bool pending_sysexit : 1; ///< is this thread mid-syscall?
  bool ignoring : 1;        ///< has the spy told us to ignore syscalls?
  int exit_status;          ///< exit status on completion
} thread_t;

/// resume a stopped thread, running it until the next event
///
/// @param thread Thread to resume
/// @return 0 on success or an errno on failure
INTERNAL int thread_cont(thread_t thread);

/// resume a stopped thread, forwarding it the given signal
///
/// If `sig` is 0, no signal will be forwarded.
///
/// @param thread Thread to resume
/// @param sig Signal to forward
/// @return 0 on success or an errno on failure
INTERNAL int thread_signal(thread_t thread, int sig);

/// resume a stopped thread, running it until the next syscall
///
/// @param thread Thread to resume
/// @return 0 on success or an errno on failure
INTERNAL int thread_syscall(thread_t thread);

/// resume a thread, detaching from tracing it
///
/// After a successful call to this function, the target thread remains our
/// child and needs to eventually be `wait`-ed on.
///
/// @param thread Thread to resume
/// @param sig Signal to forward
/// @return 0 on success or an errno on failure
INTERNAL int thread_detach(thread_t thread, int sig);

/// register thread exit
///
/// @param thread Thread to update
/// @param exit_status Exit status to save
INTERNAL void thread_exit(thread_t *thread, int exit_status);

/// register a new open file descriptor
///
/// @param proc Process to register the file descriptor with
/// @param fd File descriptor number
/// @param path Absolute path to the open file/directory
/// @return 0 on success or an errno on failure
INTERNAL int proc_fd_new(proc_t *proc, int fd, const char *path);

/// lookup a file descriptor
///
/// @param proc Process in whose context to search
/// @param fd Number of the descriptor
/// @return The found descriptor or `NULL` if not found
INTERNAL const fd_t *proc_fd(const proc_t *proc, int fd);

/// reset the file descriptor table
///
/// @param proc Process whose table to reset
INTERNAL void proc_fds_free(proc_t *proc);

/// deallocate resources for a process
///
/// This only frees up resources if there are no remaining threads referencing
/// it.
///
/// @param proc Process to free
INTERNAL void proc_free(proc_t *proc);
