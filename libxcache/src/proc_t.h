#pragma once

#include "../../common/compiler.h"
#include <sys/types.h>
#include <xcache/cmd.h>

/// a subprocess being traced
typedef struct {
  int outfd[2]; ///< pipe for communicating stdout content
  int errfd[2]; ///< pipe for communicating stderr content

  pid_t pid; ///< process ID of the child
} proc_t;

/** create a new process
 *
 * \param proce [out] Created process on success
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_new(proc_t *proc);

/** start a process running
 *
 * This function `fork`s.
 *
 * \param proc Store for started process ID
 * \param cmd Command to start running
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_start(proc_t *proc, const xc_cmd_t cmd);

/** execute a process
 *
 * This function is intended to be called by a subprocess/tracee. On failure, it
 * calls `exit` with an errno.
 *
 * \param proc Process to run
 * \param cmd Command describing what to `exec`
 */
INTERNAL _Noreturn void proc_exec(const proc_t *proc, const xc_cmd_t cmd);

/** unceremoniously terminate a child
 *
 * This is a no-op if the child has already terminated.
 *
 * \param proc Process to terminate
 */
INTERNAL void proc_end(proc_t *proc);

/** destroy a process
 *
 * \param proc Process to destroy
 */
INTERNAL void proc_free(proc_t proc);
