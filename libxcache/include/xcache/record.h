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

/// run a command and monitor its behaviour
///
/// This function `fork`s and spawns background threads. It attempts to join all
/// these subprocesses and threads before returning.
///
/// `mode` is expected to be a bitmask of `xc_record_mode_t` values.
///
/// The `exit_status` is always set when the command runs to completion. That
/// is, it is set both when 0 is returned and when `ECHILD` is returned
/// indicating the command could not be recorded due to it doing something
/// unsupported.
///
/// @param db Database to record results into
/// @param cmd Command to run
/// @param mode Acceptable modes in which to record
/// @param exit_status [out] Exit status of the command on success
/// @return 0 on success or an errno on failure
XCACHE_API int xc_record(xc_db_t *db, const xc_cmd_t cmd, unsigned mode,
                         int *exit_status);

#ifdef __cplusplus
}
#endif
