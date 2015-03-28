#ifndef _XCACHE_TRACE_H_
#define _XCACHE_TRACE_H_

#include "collection/list.h"
#include <linux/limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

/* A process being tracked. The processes we deal with are always either the
 * root (the original process that was forked off to be traced) or one of the
 * root's children.
 */
typedef struct {

    /* Process ID (PID) of this process. */
    pid_t pid;

    /* The current state of this process. This 'state' value is a bit fuzzy in
     * that it's only updated when we notice a relevant event. That is, if you
     * asynchronously read this state out-of-band, it is likely to be an
     * inaccurate representation of what the process is currently doing.
     */
    enum {
        IN_USER,     /* Executing in userspace */
        SYSENTER,    /* Interrupted at kernel entry */
        IN_KERNEL,   /* Executing in the kernel */
        SYSEXIT,     /* Interrupted at kernel exit */
        TERMINATED,  /* Exited */
        FINALISED,   /* Exited and xcache metadata cleaned up */
    } state;

    /* Current working directory of the process. */
    char cwd[PATH_MAX];

} proc_t;

/* Representation of a process to be traced. */
typedef struct {

    /* The exit code of the process. This only gets filled in after the process
     * exits.
     */
    unsigned char exit_status;

    /* The initial process that is executed. If the process never forks, this
     * will be the only PID we track.
     */
    proc_t root;

    /* A list of proc_ts that are children forked off from 'root'. We need to
     * track these similarly to `strace -f` in order to keep tabs on everything
     * the program is doing.
     */
    list_t children;

    /* Temporary files that are used to store the contents of stdout and
     * stderr while tracing a program. These are created when we start tracing
     * a program. We could collect this data in memory, but there's no real
     * advantage to this when we need to eventually store these as files in the
     * cache anyway.
     */
    char *outfile, *errfile;

    /* File descriptors to the above two opened for writing upon creation. */
    int outfd, errfd;

    /* Various pipes used by the hook to communicate to or from another
     * thread. Note that the hook is reading from multiple of these and uses
     * the final one to detect when the main thread wants it to exit.
     */
    int stdout_pipe[2]; /* Written by the tracee, read by the hook. */
    int stderr_pipe[2]; /* Written by the tracee, read by the hook. */
    int msg_pipe[2];    /* Written by libhook, read by the hook. */
    int sig_pipe[2];    /* Written by the xcache main thread, read by the hook. */

    /* Subordinate thread to read from various pipes written by the tracee. We
     * do this in a subordindate thread to avoid interferring with the
     * monitoring of syscalls in the main thread. The main thread eventually
     * writes to 'sig_pipe' to tell this thread to exit.
     */
    pthread_t hook;

    /* Environment variables read by the tracee. This is populated by the hook,
     * based on messages it receives through 'msg_pipe' from libhook
     * injected into the tracee.
     */
    dict_t env;

} target_t;

/* A detected syscall from the tracee. */
typedef struct {

    /* The PID that made this syscall. */
    proc_t *proc;

    /* The syscall number. Note that this is architecture *and* kernel
     * version dependent.
     */
    long call;

    /* Whether this event represents a syscall entry (user-to-kernel
     * transition) or syscall exit (kernel-to-user transition).
     */
    bool enter;

    /* The return value of the syscall. Note that this is irrelevant if this is
     * a syscall entry.
     */
    long result;

} syscall_t;

/* Setup a new target for tracing. This starts the given target executing and
 * sets everything up so we can wait for the process's next syscall.
 *  t - Tracking structure to populate. This needs to passed onwards to further
 *    function calls in this module.
 *  argv - The process to trace. This needs to conform to the usual standard of
 *    being a NULL-terminated array of arguments.
 *  tracer - The path to the xcache binary itself. This should be used by the
 *    called to indicate whether they want libhook injected into the
 *    target. If you pass a NULL pointer it will not be injected. The reason
 *    for not injecting libhook is typically that the target does not
 *    link against libdl, which makes library hooking a bit difficult.
 * Returns 0 on success.
 */
int trace(target_t *t, const char **argv, const char *tracer);

/* Wait for the next syscall from the given target and return it. Note that
 * when this function returns the target will be blocked (SIGTRAP) and you will
 * need to call acknowledge_syscall() to resume the target.
 */
syscall_t *next_syscall(target_t *tracee);

/* Resume the target who initiated the given syscall. Returns 0 on success. */
int acknowledge_syscall(syscall_t *syscall);

/* Detach from the target and wait for it to finish. You should call this when
 * you're bailing out of tracing a given target and just want to let it resume
 * its execution unmonitored. Note that this function assumes that any or all
 * of the root and children of this target may currently be trapped, and it
 * takes care of unblocking them.
 */
int complete(target_t *tracee);

/* Clean up the target and deallocate associated resources. This function
 * should not be called without calling complete() first. Returns 0 on success.
 */
int delete(target_t *tracee);

/* Retrieve a string argument to a syscall. */
char *syscall_getstring(syscall_t *syscall, int arg);

/* Retrieve an integral argument to a syscall. */
long syscall_getarg(syscall_t *syscall, int arg);

/* Retrieve the path of a file descriptor argument to a syscall. */
char *syscall_getfd(syscall_t *syscall, int arg);

/* XXX: consider removing these. */
const char *get_stdout(target_t *tracee);
const char *get_stderr(target_t *tracee);

#endif
