#pragma once

#include "../../common/compiler.h"
#include "input.h"
#include "list.h"
#include "output_t.h"
#include "tee_t.h"
#include "thread_t.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <xcache/cmd.h>
#include <xcache/record.h>

/// a (potentially multi-threaded, multi-process) target being traced
typedef struct {
  xc_record_mode_t mode;

  tee_t *t_out; ///< pipe for communicating stdout content
  tee_t *t_err; ///< pipe for communicating stderr content

  int exec_status[2]; ///< pipe for propagating initial `execve` result

  int exit_status; ///< exit status on completion

  LIST(thread_t *) threads; ///< threads belonging to this target

  inputs_t inputs; ///< input actions observed

  /// list of output actions observed
  ///
  /// For `OUT_WRITE` items, the `cached_copy` member is not populated. That
  /// will be done during finalisation, when the trace record is being written.
  LIST(output_t) outputs;
} inferior_t;

/// create a new inferior to be traced
///
/// @param inf [out] Created inferior on success
/// @param mode Recording mode to use
/// @param trace_root Absolute path to trace output directory
/// @return 0 on success or an errno on failure
INTERNAL int inferior_new(inferior_t *inf, unsigned mode,
                          const char *trace_root);

/// start a process running
///
/// This function `fork`s. This is only expected to be called for the initial
/// process in an inferior.
///
/// \param inf Tracee container for the new process
/// \param cmd Command to start running
/// \param preload_prepend When setting up the spy, preprend to `$LD_PRELOAD`
///   instead of appending
/// \return 0 on success or an errno on failure
INTERNAL int inferior_start(inferior_t *inf, const xc_cmd_t cmd,
                            bool preload_prepend);

/// execute a process
///
/// This function is intended to be called by a subprocess/tracee. On failure,
/// it calls `exit` with an errno.
///
/// @param inf Tracee container for the new process
/// @param cmd Command describing what to `exec`
/// @param ld_preload Value to set `$LD_PRELOAD` to prior to exec
INTERNAL _Noreturn void inferior_exec(inferior_t *inf, const xc_cmd_t cmd,
                                      const char *ld_preload);

/// append a new input
///
/// @param inf Inferior to append input to
/// @param input Input to append
/// @return 0 on success or an errno on failure
INTERNAL int inferior_input_new(inferior_t *inf, input_t input);

/// append a new output
///
/// @param inf Inferior to append output to
/// @param output Output to append
/// @return 0 on success or an errno on failure
INTERNAL int inferior_output_new(inferior_t *inf, const output_t output);

/// learn the existence of a new thread
///
/// @param inf Inferior who just spawned a new thread
/// @param parent The thread who created the new one
/// @param child The TID of the new thread
/// @return 0 on success or an errno on failure
INTERNAL int inferior_spawn(inferior_t *inf, thread_t *parent, pid_t child);

/// write out a completed inferior’s result to a trace file
///
/// @param inf Completed inferior
/// @param cmd Command that initiated this process
/// @param trace_root Directory in which to write the trace file
/// @return 0 on success or an errno on failure
INTERNAL int inferior_save(inferior_t *proc, const xc_cmd_t cmd,
                           const char *trace_root);

/// SIGKILL all threads
///
/// @param inf Inferior whose threads to signal
INTERNAL void inferior_kill(inferior_t *inf);

/// resume a stopped thread
///
/// @param inf Home of the thread
/// @param subject Thread to resume
/// @param sig Optional signal to forward during resumption (0 == none)
/// @return 0 on success or an errno on failure
INTERNAL int inferior_thread_continue(const inferior_t *inf,
                                      const thread_t *subject, int sig);

/// mark a thread as exited and deallocate it
///
/// @param inf Inferior who owns the thread
/// @param exiter Thread exiting
/// @param exit_status The exit status of the thread
INTERNAL void inferior_thread_exit(inferior_t *inf, thread_t *exiter,
                                   int exit_status);

/// deallocate resources associated with an inferior
///
/// @param inf Inferior to free
INTERNAL void inferior_free(inferior_t *inf);
