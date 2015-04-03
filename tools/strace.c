/* A basic implementation of strace for debugging purposes. */

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

/* Entry point for child (target) execution */
static int child(char **argv) {
    /* Instrument ourselves */
    long r = ptrace(PTRACE_TRACEME);
    if (r != 0)
        return (int)r;

    /* Wait for our parent (tracer) to attach */
    raise(SIGSTOP);

    /* OK, let's go */
    return execvp(argv[0], argv);
}

/* Get default ptrace options */
static void *ptrace_options(bool trace_children) {
    /* Give us the ability to detect syscalls */
    long options = PTRACE_O_TRACESYSGOOD;

    if (trace_children)
        options |= PTRACE_O_TRACEFORK|
                   PTRACE_O_TRACEVFORK|
                   PTRACE_O_TRACECLONE;

    return (void*)options;
}

/* Attach to the given child. Note, this relies on a coordination sequence with
 * `child` above.
 */
static int attach(pid_t target, bool trace_children) {
    int status;
    if (waitpid(target, &status, 0) == -1) {
        fprintf(stderr, "failed to wait on child\n");
        return -1;
    }

    if (WIFEXITED(status)) {
        fprintf(stderr, "child immediately exited\n");
        return -1;
    }

    if (ptrace(PTRACE_SETOPTIONS, target, NULL,
            ptrace_options(trace_children)) != 0) {
        fprintf(stderr, "failed to set initial tracing options\n");
        return -1;
    }

    if (ptrace(PTRACE_SYSCALL, target, NULL, NULL) != 0) {
        fprintf(stderr, "failed initial resume of target\n");
        return -1;
    }

    return 0;
}

/* Functionality for retrieving the syscall number of the current syscall */
static int get_syscall_no(pid_t pid) {
#define OFFSET(reg) \
    (__builtin_offsetof(struct user, regs) + \
     __builtin_offsetof(struct user_regs_struct, reg))

#ifndef REG_SYSNO
    /* Assume x86_64 if the user didn't tell us */
    #define REG_SYSNO orig_rax
#endif

    return (int)ptrace(PTRACE_PEEKUSER, pid, OFFSET(REG_SYSNO));
}

/* Trace a child, documenting its signals and syscalls */
static int trace(pid_t root) {
    while (true) {
        int status;
        pid_t pid = wait(&status);

        if (pid == -1) {
            fprintf(stderr, "failed wait()\n");
            continue;
        }

        if (WIFEXITED(status)) {
            printf("%d exited\n", pid);
            /* If the root exited, terminate tracing */
            if (pid == root)
                return WEXITSTATUS(status);
        } else if (WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);

            if (sig == (SIGTRAP|0x80)) { /* see `man ptrace` on sysgood */
                printf("%d syscall %d\n", pid, get_syscall_no(pid));
                ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            } else {
                printf("%d received signal %d\n", pid, sig);
                ptrace(PTRACE_SYSCALL, pid, NULL, sig);
            }
        } else {
            fprintf(stderr, "%d received unhandled stop event\n", pid);
        }
    }
}

int main(int argc, char **argv) {
    bool trace_children = false; /* default */

    /* The child's command line should have followed our invocation */
    char **child_argv = argv + 1;

    if (argc > 1 && strcmp(argv[1], "-f") == 0) {
        trace_children = true;
        child_argv++;
    }

    if (child_argv[0] == NULL) {
        fprintf(stderr, "no tracee provided\n");
        return -1;
    }

    pid_t pid = fork();
    switch (pid) {
        case -1:
            fprintf(stderr, "fork failed\n");
            return -1;

        case 0: /* child */
            return child(child_argv);

    }

    /* Only the parent will reach here */
    assert(pid > 0);

    if (attach(pid, trace_children) != 0)
        return -1;

    return trace(pid);
}
