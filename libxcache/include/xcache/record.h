#pragma once

#include <xcache/cmd.h>
#include <xcache/db.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

/// tracing technique to use when recording
///
/// Linux has evolved over the years, adding improved ways to do the kind of
/// tracing libxcache needs. The improvements, at a high level, reduce the
/// number of times we need to stop a tracee per-syscall:
///
///   ┌──────────────────┬────────────────────────┬───────┬──────────────────┐
///   │ value            │ description            │ stops │ availability     │
///   ├──────────────────┼────────────────────────┼───────┼──────────────────┤
///   │ XC_SYSCALL       │ ptrace                 │ 2     │ *                │
///   │ XC_EARLY_SECCOMP │ +PTRACE_O_TRACESECCOMP │ 0-3   │ Linux [3.5, 4.8) │
///   │ XC_LATE_SECCOMP  │ +PTRACE_O_TRACESECCOMP │ 0-2   │ Linux ≥ 4.8      │
///   └──────────────────┴────────────────────────┴───────┴──────────────────┘
///
/// Basically you want to use the latest one of these available to you.
typedef enum {
  XC_SYSCALL = 1,       ///< vanilla ptrace
  XC_EARLY_SECCOMP = 2, ///< seccomp filter, stopping pre-syscall
  XC_LATE_SECCOMP = 4,  ///< seccomp filter, stopping mid-syscall

  /// alias for using any available mode
  XC_MODE_AUTO = XC_SYSCALL | XC_EARLY_SECCOMP | XC_LATE_SECCOMP,
} xc_record_mode_t;

/// which recording techniques are available?
///
/// @param request A mask of `xc_record_mode_t` values of interest
/// @return The subset of `request` bits that map to available modes
XCACHE_API unsigned xc_record_modes(unsigned request);

/// result of an `xc_record` call
typedef struct {
  /// result of calling `execve` to run the given command
  ///
  /// `exec_status` is set when `execve` or one of the other “initialisation”
  /// steps fails. This allows an `xc_record` caller to discriminate between
  /// something going wrong during tracing and something going wrong during
  /// starting the tracee.
  ///
  /// This member is only meaningful if `xc_record` returns 0.
  int exec_status;

  /// the command’s exit status
  ///
  /// This member is only meaningful if `xc_record` returns 0 and `exec_status`
  /// is 0.
  int exit_status;

  /// result of tracing
  ///
  /// This member will is only meaningful if `xc_record` returns 0.
  int trace_status;
} xc_record_t;

/// run a command and monitor its behaviour
///
/// This function `fork`s and spawns background threads. It attempts to join all
/// these subprocesses and threads before returning.
///
/// `mode` is expected to be a bitmask of `xc_record_mode_t` values.
///
/// `status` is set as described above in `xc_record_t`’s documentation.
///
/// @param db Database to record results into
/// @param cmd Command to run
/// @param mode Acceptable modes in which to record
/// @param status [out] Result of tracing actions
/// @return 0 on success or an errno on failure
XCACHE_API int xc_record(xc_db_t *db, const xc_cmd_t cmd, unsigned mode,
                         xc_record_t *status);

#ifdef __cplusplus
}
#endif
