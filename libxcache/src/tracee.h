#pragma once

#include "channel.h"
#include "macros.h"
#include <sys/types.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

/// encapsulation of a subprocess being traced
typedef struct {

  /// process being traced
  const xc_proc_t *proc;

  /// identifier of the subprocess (only > 0 if it is running)
  pid_t pid;

  /// file descriptor to the process
  int pidfd;

  /// pipe for stdout
  int out[2];

  /// pipe for stderr
  int err[2];

  /// pipe for signalling any failure of the tracee to the tracer
  channel_t msg;
} tracee_t;

/// initialise a tracee
///
/// \param tracee [out] Initialised tracee on success
/// \param proc Process about to be traced
/// \return 0 on success or an errno on failure
INTERNAL int tracee_init(tracee_t *tracee, const xc_proc_t *proc);

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

/// clear fields of a tracee and deallocate underlying resources
///
/// Idempotent; it is OK to call this multiple times on the same tracee.
///
/// \param tracee Tracee to clear
INTERNAL void tracee_deinit(tracee_t *tracee);
