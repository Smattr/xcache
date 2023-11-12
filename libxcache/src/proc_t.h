#pragma once

#include "../../common/compiler.h"
#include "action_t.h"
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

  int outfd[2]; ///< pipe for communicating stdout content
  int errfd[2]; ///< pipe for communicating stderr content

  char *cwd; ///< current working directory

  pid_t pid;                ///< process ID of the child
  bool pending_sysexit : 1; ///< is this process mid-syscall?

  fd_t **fds;   ///< file descriptor table
  size_t n_fds; ///< number of entries in `fds`

  action_t *actions; ///< list of actions observed
} proc_t;

/** create a new process
 *
 * \param proc [out] Created process on success
 * \param mode Acceptable modes
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_new(proc_t *proc, unsigned mode);

/** register a new open file descriptor
 *
 * \param proc Process to register the file descriptor with
 * \param fd File descriptor number
 * \param path Absolute path to the open file/directory
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_fd_new(proc_t *proc, int fd, const char *path);

/** reset the file descriptor table
 *
 * \param proc Process whose table to reset
 */
INTERNAL void proc_fds_free(proc_t *proc);

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

/** destroy a process
 *
 * \param proc Process to destroy
 */
INTERNAL void proc_free(proc_t proc);
