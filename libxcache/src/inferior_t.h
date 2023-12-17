#pragma once

#include <xcache/cmd.h>
#include <xcache/record.h>
#include "tee_t.h"
#include "input_t.h"
#include "proc_t.h"
#include <stddef.h>
#include "output_t.h"
#include "../../common/compiler.h"

/// a (potentially multi-threaded, multi-process) target being traced
typedef struct {
  xc_record_mode_t mode;

  int fate; ///< errno if tracing has already failed

  tee_t *t_out; ///< pipe for communicating stdout content
  tee_t *t_err; ///< pipe for communicating stderr content

  int proccall[2]; ///< pipe for the libxcache-spy to message us

  proc_t *procs; ///< processes belonging to this target
  size_t n_procs; ///< number of entries in `procs`
  size_t c_procs; ///< number of allocated slots in `procs`

  input_t *inputs; ///< list of input actions observed
  size_t n_inputs; ///< number of entries in `inputs`
  size_t c_inputs; ///< number of allocated slots in `inputs`

  /** list of output actions observed
   *
   * For `OUT_WRITE` items, the `cached_copy` member is not populated. That
   * will be done during finalisation, when the trace record is being written.
   */
  output_t *outputs;
  size_t n_outputs; ///< number of entries in `outputs`
  size_t c_outputs; ///< number of allocated slots in `outputs`
} inferior_t;

/** create a new inferior to be traced
 *
 * \param inf [out] Created inferior on success
 * \param mode Recording mode to use
 * \param trace_root Absolute path to trace output directory
 * \return 0 on success or an errno on failure
 */
INTERNAL int inferior_new(inferior_t *inf, unsigned mode, const char *trace_root);

/** start a process running
 *
 * This function `fork`s. This is only expected to be called for the initial
 * process in an inferior.
 *
 * \param inf Tracee container for the new process
 * \param cmd Command to start running
 * \return 0 on success or an errno on failure
 */
INTERNAL int inferior_start(inferior_t *inf, const xc_cmd_t cmd);

/** execute a process
 *
 * This function is intended to be called by a subprocess/tracee. On failure, it
 * calls `exit` with an errno.
 *
 * \param inf Tracee container for the new process
 * \param proc Process to run
 * \param cmd Command describing what to `exec`
 * \param spy Absolute path to parasite library to inject
 */
INTERNAL _Noreturn void inferior_exec(const inferior_t *inf, const proc_t *proc, const xc_cmd_t cmd,
                                  const char *spy);

/** append a new input
 *
 * \param inf Inferior to append input to
 * \param input Input to append
 * \return 0 on success or an errno on failure
 */
INTERNAL int inferior_input_new(inferior_t *inf, const input_t input);

/** append a new output
 *
 * \param inf Inferior to append output to
 * \param output Output to append
 * \return 0 on success or an errno on failure
 */
INTERNAL int inferior_output_new(inferior_t *inf, const output_t output);

/** write out a completed inferior’s result to a trace file
 *
 * \param inf Completed inferior
 * \param cmd Command that initiated this process
 * \param trace_root Directory in which to write the trace file
 * \return 0 on success or an errno on failure
 */
INTERNAL int inferior_save(inferior_t *proc, const xc_cmd_t cmd,
                       const char *trace_root);

/** SIGKILL all processes
 *
 * After a call to this function, the inferior’s (possibly zombie) processes
 * remain our children and need to be `wait`-ed on.
 *
 * \param inf Inferior whose processes to signal
 */
INTERNAL void inferior_kill(inferior_t *inf);

INTERNAL void inferior_free(inferior_t *inf);
