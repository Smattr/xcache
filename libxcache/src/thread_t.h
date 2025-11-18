#pragma once

#include "../../common/compiler.h"
#include "fd.h"
#include "fs.h"
#include "proc.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <xcache/cmd.h>
#include <xcache/record.h>

/// flags recorded from a `clone`/`clone3`/`fork`/`vfork`
typedef struct {
  /// flags populated?
  ///
  /// This is used to model an optional type. Essentially a flag to indicate
  /// when the value of this struct is “unset”.
  bool set : 1;

  bool clone_files : 1;  ///< was `CLONE_FILES` set?
  bool clone_fs : 1;     ///< was `CLONE_FS` set?
  bool clone_thread : 1; ///< was `CLONE_THREAD` set?
} clone_flags_t;

/// a thread within a process
typedef struct {
  pid_t id;                  ///< thread identifier
  proc_t *proc;              ///< containing process
  fs_t *fs;                  ///< filesystem
  fds_t *fd;                 ///< file descriptor table
  bool pending_sigstop : 1;  ///< do we need to acknowledge a future `SIGSTOP`?
  bool pending_sysexit : 1;  ///< is this thread mid-syscall?
  bool ignoring : 1;         ///< has the spy told us to ignore syscalls?
  clone_flags_t clone_flags; ///< options observed from last clone() syscall
  int *exit_status;          ///< where to write exit status on completion
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
/// @param cont Use `PTRACE_CONT` to resume (instead of `PTRACE_SYSCALL`)?
/// @return 0 on success or an errno on failure
INTERNAL int thread_signal(thread_t thread, int sig, bool cont);

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
