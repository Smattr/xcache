#define _GNU_SOURCE
#include "arch_syscall.h"
#include <assert.h>
#include "collection/list.h"
#include "environ.h"
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/limits.h>
#include "log.h"
#include <pthread.h>
#include "ptrace-wrapper.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tee.h"
#include "trace.h"
#include <unistd.h>
#include "util.h"

struct proc {
    pid_t pid;
    enum {
        IN_USER,
        SYSENTER,
        IN_KERNEL,
        SYSEXIT,
        TERMINATED,
        FINALISED,
    } state;
};

struct tracee {
    unsigned char exit_status;
    proc_t root;

    list_t children;

    tee_t *out, *err;
    char *outfile, *errfile;
};

static int proc_cmp(void *proc, void *pid) {
    proc_t *p = (proc_t*)proc;
    return p->pid != (pid_t)(unsigned long)pid;
}

/* Find the accompanying getenv hook library. We assume it lives in the same
 * directory as the xcache binary.
 */
static char *locate_hooklib(const char *exe) {
    char *resolved = malloc(sizeof(char) * (PATH_MAX + 1));
    if (resolved == NULL)
        return NULL;

    if (realpath(exe, resolved) == NULL)
        return NULL;

    char *root = dirname(resolved);
    free(resolved);

    return aprintf("%s/libhookgetenv.so", root);
}

tracee_t *trace(const char **argv, const char *tracer) {
    tracee_t *t = calloc(1, sizeof(*t));
    if (t == NULL)
        return NULL;

    if (list(&t->children, proc_cmp) != 0)
        goto fail;

    int stdout2 = STDOUT_FILENO;
    t->out = tee_create(&stdout2);
    if (t->out == NULL)
        goto fail;

    int stderr2 = STDERR_FILENO;
    t->err = tee_create(&stderr2);
    if (t->err == NULL)
        goto fail;

    t->root.pid = fork();
    switch (t->root.pid) {

        case 0: {
            /* We are the child. */
            if (dup2(stdout2, STDOUT_FILENO) == -1 ||
                    dup2(stderr2, STDERR_FILENO) == -1)
                exit(-1);

            /* Copy and extend our environment to LD_PRELOAD a helper library
             * into the target. The idea behind this is to setup a channel
             * between xcache and the tracee for communicating extra
             * information beyond syscalls. Note that if any of this fails, we
             * just ignore it and continue without the extra library. This
             * functionality is not critical.
             */
            char **env = environ;
            if (tracer != NULL) {
                char *lib = locate_hooklib(tracer);
                if (lib != NULL) {
                    env = env_ld_preload(lib);
                    free(lib);
                }
            }

            long r = pt_traceme();
            if (r != 0)
                exit(-1);
            execvpe(argv[0], (char**)argv, env);

            /* Exec failed. */
            exit(-1);
        } case -1: {
            /* Fork failed. */
            DEBUG("failed initial fork (%d)\n", errno);
            goto fail;
        }
    }

    assert(t->root.pid > 0);
    int status;
    waitpid(t->root.pid, &status, 0);
    if (WIFEXITED(status)) {
        /* Either ptrace or exec failed in the child. */
        DEBUG("tracee exited immediately\n");
        goto fail;
    }

    long r = pt_tracechildren(t->root.pid);
    if (r != 0) {
        DEBUG("failed to set tracing to catch forks (%d)\n", errno);
        goto fail;
    }

    r = pt_runtosyscall(t->root.pid);
    if (r != 0) {
        DEBUG("failed first resume of tracee (%d)\n", errno);
        goto fail;
    }
    t->root.state = IN_USER;
    return t;

fail:
    if (t->err != NULL) {
        char *p = tee_close(t->err);
        if (p != NULL)
            unlink(p);
        free(p);
    }
    if (t->out != NULL) {
        char *p = tee_close(t->out);
        if (p != NULL)
            unlink(p);
        free(p);
    }
    free(t);
    return NULL;
}

#define OFFSET(reg) \
    (__builtin_offsetof(struct user, regs) + \
     __builtin_offsetof(struct user_regs_struct, reg))

static long syscall_number(pid_t pid) {
    return pt_peekreg(pid, OFFSET(REG_SYSNO));
}

static long syscall_result(pid_t pid) {
    return pt_peekreg(pid, OFFSET(REG_RESULT));
}

static long register_offset(int arg) {
    switch (arg) {

        /* Depending on our architecture, we may only have a subset of the
         * following registers.
         */
#ifdef REG_ARG1
        case 1:
            return OFFSET(REG_ARG1);
#endif
#ifdef REG_ARG2
        case 2:
            return OFFSET(REG_ARG2);
#endif
#ifdef REG_ARG3
        case 3:
            return OFFSET(REG_ARG3);
#endif
#ifdef REG_ARG4
        case 4:
            return OFFSET(REG_ARG4);
#endif
#ifdef REG_ARG5
        case 5:
            return OFFSET(REG_ARG5);
#endif
#ifdef REG_ARG6
        case 6:
            return OFFSET(REG_ARG6);
#endif
        default:
            DEBUG("attempt to access unsupported argument register %d\n", arg);
            return -1;
    }
}

char *syscall_getstring(syscall_t *syscall, int arg) {
    assert(arg > 0);

    long offset = register_offset(arg);
    if (offset == -1)
        return NULL;

    return pt_peekstring(syscall->proc->pid, offset);
}

long syscall_getarg(syscall_t *syscall, int arg) {
    long offset = register_offset(arg);
    if (offset == -1)
        return -1;
    return pt_peekreg(syscall->proc->pid, offset);
}

syscall_t *next_syscall(tracee_t *tracee) {
    assert(tracee != NULL);
    if (tracee->root.state != IN_USER && tracee->root.state != IN_KERNEL) {
        DEBUG("attempt to retrieve a syscall from a stopped process\n");
        return NULL;
    }

retry:;
    int status;
    pid_t pid = wait(&status);

    if (WIFEXITED(status)) {
        if (pid != tracee->root.pid) {
            /* A forked child exited. */
            IDEBUG("child %d exited\n", pid);
            proc_t *p = list_remove(&tracee->children,
                (void*)(uintptr_t)pid);
            assert(p != NULL && p->pid == pid);
            free(p);
            goto retry;
        }
        /* In the following we are assuming a well behaved tracee that waits on
         * all its forked children. That is, exit of the root process implies
         * the entire operation has completed.
         */
        tracee->root.state = TERMINATED;
        tracee->exit_status = WEXITSTATUS(status);
        DEBUG("tracee exited with status %d\n", tracee->exit_status);
        return NULL;
    }

    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP &&
            ((status >> 8) == (SIGTRAP|PTRACE_EVENT_FORK << 8) ||
             (status >> 8) == (SIGTRAP|PTRACE_EVENT_VFORK << 8) ||
             (status >> 8) == (SIGTRAP|PTRACE_EVENT_CLONE << 8))) {
        /* The target called fork (or a cousin of). Unless I've missed
         * something in the ptrace docs, the only way to also trace forked
         * children is to set PTRACE_O_FORK and friends on the root process.
         * Unfortunately the result of this is that we get two events that tell
         * us the same thing: a SIGTRAP in the parent on fork (this case) and a
         * SIGSTOP in the child before execution (handled below). It's simpler
         * to just ignore the SIGTRAP in the parent and start tracking the
         * child when we receive its initial SIGSTOP.
         */
        long r = pt_runtosyscall(pid);
        if (r != 0)
            DEBUG("failed to resume parent process %d (errno: %d)\n", pid,
                errno);
        /* We don't yet have a syscall to return to the caller, so try again.
         */
        goto retry;
    }

    assert(WIFSTOPPED(status));
    proc_t *p;
    if (pid == tracee->root.pid)
        p = &tracee->root;
    else
        p = list_find(&tracee->children, (void*)(uintptr_t)pid);
    if (p == NULL) {
        /* We've hit a signal in a new (untraced) process. This is the first
         * we've seen of a forked child process, so let's start tracing it.
         */
        assert(WSTOPSIG(status) == SIGSTOP);
        p = malloc(sizeof(*p));
        if (p == NULL)
            return NULL;
        p->pid = pid;
        p->state = IN_USER;
        list_add(&tracee->children, p);
        long r = pt_runtosyscall(pid);
        if (r != 0)
            DEBUG("warning: failed to continue forked child %d (errno: %d)\n",
                pid, errno);
        /* We still don't have a syscall for the caller, so try again. */
        goto retry;
    }
    syscall_t *s = malloc(sizeof(*s));
    if (s == NULL)
        return NULL;
    s->proc = p;
    s->call = syscall_number(pid);
    s->enter = (p->state == IN_USER);
    if (s->enter && syscall_result(pid) != -ENOSYS)
        /* Maybe not especially relevant, but the libc syscall entry stubs
         * setup -ENOSYS in the syscall result register. This means we can
         * detect a 'raw' syscall entry by the absence of this value. Note, for
         * the purposes of this tool we don't really care how the user enters
         * the kernel.
         */
        IDEBUG("warning: target appears to have invoked syscall %ld directly "
            "(not via libc stubs)\n", syscall_number(pid));
    if (!s->enter)
        s->result = syscall_result(pid);
    if (p->state == IN_USER)
        p->state = SYSENTER;
    else {
        assert(p->state == IN_KERNEL);
        p->state = SYSEXIT;
    }

    return s;
}

int acknowledge_syscall(syscall_t *syscall) {
    assert(syscall->proc->state == SYSENTER ||
           syscall->proc->state == SYSEXIT);
    long r = pt_runtosyscall(syscall->proc->pid);
    if (r != 0)
        DEBUG("failed to resume process (%d)\n", errno);
    if (syscall->proc->state == SYSENTER)
        syscall->proc->state = IN_KERNEL;
    else
        syscall->proc->state = IN_USER;
    free(syscall);
    return (int)r;
}

static int finish(pid_t pid) {
    while (true) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFSTOPPED(status)) {
            pt_passthrough(pid, status);
        } else if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            assert(WIFSIGNALED(status));
            return (1 << 7)|WTERMSIG(status);
        }
    }
}

void unblock(proc_t *proc) {
    if (proc->state == SYSENTER || proc->state == SYSEXIT)
        pt_continue(proc->pid);
}

int complete(tracee_t *tracee) {
    if (tracee->root.state != TERMINATED) {
        void dealloc(void *data, void *_ __attribute__((unused))) {
            proc_t *p = data;
            unblock(p);
            pt_detach(p->pid);
            free(p);
        }
        list_foreach(&tracee->children, dealloc, NULL);
        list_destroy(&tracee->children);
        tracee->root.state = TERMINATED;
        unblock(&tracee->root);
        tracee->exit_status = finish(tracee->root.pid);
    }
    assert(tracee->outfile == NULL);
    tracee->outfile = tee_close(tracee->out);
    assert(tracee->errfile == NULL);
    tracee->errfile = tee_close(tracee->err);
    tracee->root.state = FINALISED;
    return tracee->exit_status;
}

const char *get_stdout(tracee_t *tracee) {
    assert(tracee->root.state == FINALISED);
    return tracee->outfile;
}

const char *get_stderr(tracee_t *tracee) {
    assert(tracee->root.state == FINALISED);
    return tracee->errfile;
}

int delete(tracee_t *tracee) {
    if (tracee->errfile != NULL)
        free(tracee->errfile);
    if (tracee->outfile != NULL)
        free(tracee->outfile);
    list_destroy(&tracee->children);
    free(tracee);
    return 0;
}
