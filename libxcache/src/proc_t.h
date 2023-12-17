#pragma once

#include "../../common/compiler.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <xcache/cmd.h>
#include <xcache/record.h>

/// a thread within a process
typedef struct {
  pid_t id; ///< thread identifier
  bool pending_sysexit : 1; ///< is this thread mid-syscall?
  bool ignoring : 1; ///< has the spy told us to ignore syscalls?
} thread_t;

/** resume a stopped thread, running it until the next event
 *
 * \param thread Thread to resume
 * \return 0 on success or an errno on failure
 */
INTERNAL int thread_cont(thread_t thread);

/** resume a stopped thread, forwarding it the given signal
 *
 * If `sig` is 0, no signal will be forwarded.
 *
 * \param thread Thread to resume
 * \param sig Signal to forward
 * \return 0 on success or an errno on failure
 */
INTERNAL int thread_signal(thread_t thread, int sig);

/** resume a stopped thread, running it until the next syscall
 *
 * \param thread Thread to resume
 * \return 0 on success or an errno on failure
 */
INTERNAL int thread_syscall(thread_t thread);

/** resume a thread, detaching from tracing it
 *
 * After a successful call to this function, the target thread remains our child
 * and needs to eventually be `wait`-ed on.
 *
 * \param thread Thread to resume
 * \param sig Signal to forward
 * \return 0 on success or an errno on failure
 */
INTERNAL int thread_detach(thread_t thread, int sig);

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

/// a process being traced
typedef struct {
  char *cwd; ///< current working directory

  pid_t id;                ///< process identifier

  thread_t *threads; ///< threads running in this process
  size_t n_threads; ///< number of entries in `threads`
  size_t c_threads; ///< number of allocated slots in `threads`

  int exit_status; ///< exit status on completion

  fd_t **fds;   ///< file descriptor table
  size_t n_fds; ///< number of entries in `fds`
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

/** register process exit
 *
 * \param proc Process to update
 * \param exit_status Exit status to save
 */
INTERNAL void proc_exit(proc_t *proc, int exit_status);

/** unceremoniously terminate a process
 *
 * This is a no-op if the process has already terminated.
 *
 * \param proc Process to terminate
 */
INTERNAL void proc_end(proc_t *proc);
