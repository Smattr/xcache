#pragma once

#include "macros.h"
#include "trace.h"
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <xcache/db_t.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

/// encapsulation of a subprocess being traced
typedef struct {

  /// process being traced
  const xc_proc_t *proc;

  /// identifier of the subprocess (only > 0 if it is running)
  pid_t pid;

  /// current working directory of the process
  char *cwd;

  /// pipe for stdout
  int out[2];

  /// pipe for stderr
  int err[2];

  /// bytes written to stdout
  FILE *out_f;
  char *out_path;

  /// bytes written to stderr
  FILE *err_f;
  char *err_path;

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

/// witness a specific syscall
INTERNAL int see_access(tracee_t *tracee, const char *pathname);
INTERNAL int see_chdir(tracee_t *tracee, int result, const char *path);
INTERNAL int see_execve(tracee_t *tracee, const char *filename);
INTERNAL int see_openat(tracee_t *tracee, int result, int dirfd,
                        const char *pathname, int flags);

/// clear fields of a tracee and deallocate underlying resources
///
/// Idempotent; it is OK to call this multiple times on the same tracee.
///
/// \param tracee Tracee to clear
INTERNAL void tracee_deinit(tracee_t *tracee);
