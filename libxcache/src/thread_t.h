#pragma once

#include "../../common/compiler.h"
#include "fd.h"
#include "fs.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <xcache/cmd.h>
#include <xcache/record.h>

/// a process being traced
typedef struct {
  pid_t id; ///< process identifier

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
  fds_t *fd;                ///< file descriptor table
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

/// deallocate resources for a process
///
/// This only frees up resources if there are no remaining threads referencing
/// it.
///
/// @param proc Process to free
INTERNAL void proc_free(proc_t *proc);
