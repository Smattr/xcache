#pragma once

#include "macros.h"
#include "trace.h"
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <xcache/db_t.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

/// a currently open file
typedef struct open_file {
  int handle;
  char *path;
  struct open_file *next;
} open_file_t;

/// encapsulation of a subprocess being traced
typedef struct {

  /// process being traced
  const xc_proc_t *proc;

  /// identifier of the subprocess (only > 0 if it is running)
  pid_t pid;

  /// current working directory of the process
  char *cwd;

  /// Open files (for resolving things like `openat`). This list may contain
  /// false positives as we do not intercept `close`.
  open_file_t *fds;

  /// pipe for stdout
  int out[2];

  /// pipe for stderr
  int err[2];

  /// bytes written to stdout
  FILE *out_f;

  /// bytes written to stderr
  FILE *err_f;

  /// trace being built up
  xc_trace_t trace;
} tracee_t;

/// initialise a tracee
///
/// \param tracee [out] Initialised tracee on success
/// \param proc Process about to be traced
/// \param db Database in which to host output artifacts
/// \return 0 on success or an errno on failure
INTERNAL int tracee_init(tracee_t *tracee, const xc_proc_t *proc, xc_db_t *db);

/// become the given subprocess
///
/// This function signals any failure through the message pipe and then calls
/// `exit`.
///
/// \param tracee Process to run
INTERNAL _Noreturn void tracee_exec(tracee_t *tracee);

/// monitor a tracee, constructing a trace of it
///
/// \param trace [out] Constructed trace on success
/// \param tracee Tracee to monitor
/// \return 0 on success or errno on failure
INTERNAL int tracee_monitor(xc_trace_t *trace, tracee_t *tracee);

/// run a loop reading and processing a tracee’s stdout, stderr
///
/// This is written in a style to match the entry point `pthread_create`
/// expects. The argument type is actually `tracee_t*` and the return type is
/// `int`.
///
/// \param arg Tracee on which to operate
/// \return 0 on success or an errno on failure
INTERNAL void *tracee_tee(void *arg);

/// restart a stopped tracee
///
/// It is assumed the tracee is in ``ptrace-stop`` when this function is called.
///
/// \param tracee Tracee to resume
/// \return 0 on success or an errno on failure
INTERNAL int tracee_resume(tracee_t *tracee);

/// restart a stopped tracee, running it until exit (or a terminating signal)
///
/// It is assumed the tracee is in `ptrace-stop` when this function is called.
/// This function detaches the tracee, so no further `seccomp` or `ptrace`
/// events will be observed.
///
/// \param tracee Tracee to resume
/// return 0 on success or an errno on failure
INTERNAL int tracee_resume_to_exit(tracee_t *tracee);

/// restart a stopped tracee, running it to the next syscall event
///
/// It is assumed the tracee is in ``ptrace-stop`` when this function is called.
///
/// \param tracee Tracee to resume
/// \return 0 on success or an errno on failure
INTERNAL int tracee_resume_to_syscall(tracee_t *tracee);

/// do any book-keeping required for seccomp (mid-syscall) stop
///
/// It is assumed a `PTRACE_EVENT_SECCOMP` has just been observed from the
/// tracee.
///
/// \param tracee Tracee to observe
/// \return 0 on success or an errno on failure
INTERNAL int syscall_middle(tracee_t *tracee);

/// do any book-keeping required for syscall exit
///
/// \param tracee Tracee that is stopped at syscall exit
/// \return 0 on success or an errno on failure
INTERNAL int syscall_end(tracee_t *tracee);

/// observe a read or write intent to a given path
///
/// \param tracee Process that executed the observed operation
/// \param dirfd openat-style relative-to handle
/// \param pathname Path read or writte
/// \return 0 on success or an errno on failure
INTERNAL int see_read(tracee_t *tracee, int dirfd, const char *pathname);
INTERNAL int see_write(tracee_t *tracee, int dirfd, const char *pathname);

/// observe successful open of a new file
///
/// \param tracee Tracee being monitored
/// \param result Handle to newly opened file
/// \param dirfd openat-style relative-to handle
/// \param pathname Path opened
/// \return 0 on success or an errno on failure
INTERNAL int see_open(tracee_t *tracee, long result, int dirfd,
                      const char *pathname);

/// clear fields of a tracee and deallocate underlying resources
///
/// Idempotent; it is OK to call this multiple times on the same tracee.
///
/// \param tracee Tracee to clear
INTERNAL void tracee_deinit(tracee_t *tracee);
