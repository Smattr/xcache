#pragma once

#include "../../common/compiler.h"
#include "input_t.h"
#include "output_t.h"
#include "tee_t.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <xcache/cmd.h>
#include <xcache/record.h>

/// an open file descriptor in a subprocess
typedef struct {
  char *path;
} fd_t;

/** create a file descriptor
 *
 * \param fd [out] Created file descriptor on success
 * \param path Absolute path of the target file/directory
 * \return 0 on success or an errno on failure
 */
INTERNAL int fd_new(fd_t **fd, const char *path);

/** destroy a file descriptor
 *
 * \param fd File descriptor to deallocate
 */
INTERNAL void fd_free(fd_t *fd);

/// a subprocess being traced
typedef struct {
  xc_record_mode_t mode;

  tee_t *t_out; ///< pipe for communicating stdout content
  tee_t *t_err; ///< pipe for communicating stderr content

  int proccall[2]; ///< pipe for subprocess to message its parent

  char *cwd; ///< current working directory

  pid_t pid;                ///< process ID of the child
  bool pending_sysexit : 1; ///< is this process mid-syscall?

  bool ignoring : 1; ///< has the spy told us to ignore syscalls?

  fd_t **fds;   ///< file descriptor table
  size_t n_fds; ///< number of entries in `fds`

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
} proc_t;

/** create a new process
 *
 * \param proc [out] Created process on success
 * \param mode Acceptable modes
 * \param trace_root Directory traces will be stored in
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_new(proc_t *proc, unsigned mode, const char *trace_root);

/** register a new open file descriptor
 *
 * \param proc Process to register the file descriptor with
 * \param fd File descriptor number
 * \param path Absolute path to the open file/directory
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_fd_new(proc_t *proc, int fd, const char *path);

/** lookup a file descriptor
 *
 * \param proc Process in whose context to search
 * \param fd Number of the descriptor
 * \return The found descriptor or `NULL` if not found
 */
INTERNAL const fd_t *proc_fd(const proc_t *proc, int fd);

/** reset the file descriptor table
 *
 * \param proc Process whose table to reset
 */
INTERNAL void proc_fds_free(proc_t *proc);

/** append a new input
 *
 * \param proc Process to append input to
 * \param input Input to append
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_input_new(proc_t *proc, const input_t input);

/** append a new output
 *
 * \param proc Process to append output to
 * \param output Output to append
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_output_new(proc_t *proc, const output_t output);

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
 * \param spy Absolute path to parasite library to inject
 */
INTERNAL _Noreturn void proc_exec(const proc_t *proc, const xc_cmd_t cmd,
                                  const char *spy);

/** resume a stopped process, running it until the next event
 *
 * \param proc Process to resume
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_cont(const proc_t proc);

/** resume a stopped process, forwarding it the given signal
 *
 * If `sig` is 0, no signal will be forwarded.
 *
 * \param proc Process to resume
 * \param sig Signal to forward
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_signal(const proc_t proc, int sig);

/** resume a stopped process, running it until the next syscall
 *
 * \param proc Process to resume
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_syscall(const proc_t proc);

/** unceremoniously terminate a child
 *
 * This is a no-op if the child has already terminated.
 *
 * \param proc Process to terminate
 */
INTERNAL void proc_end(proc_t *proc);

/** release a child, letting it run to completion
 *
 * \param proc Process to release
 */
INTERNAL void proc_detach(proc_t *proc);

/** write out a completed process result to a trace file
 *
 * \param proc Completed process
 * \param cmd Command that initiated this process
 * \param trace_root Directory in which to write the trace file
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_save(proc_t *proc, const xc_cmd_t cmd,
                       const char *trace_root);

/** destroy a process
 *
 * \param proc Process to destroy
 */
INTERNAL void proc_free(proc_t proc);
