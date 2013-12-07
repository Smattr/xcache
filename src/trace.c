#include "arch_syscall.h"
#include <assert.h>
#include "debug.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "trace.h"
#include <unistd.h>

struct proc {
    pid_t pid;
    bool running;
    bool in_syscall;
    unsigned char exit_status;
};

proc_t *trace(char **argv) {
    proc_t *p = (proc_t*)malloc(sizeof(proc_t));
    if (p == NULL) {
        return NULL;
    }

    p->pid = vfork();
    switch (p->pid) {

        case 0: {
            /* We are the child. */
            long r = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
            if (r != 0) {
                exit(-1);
            }
            execvp(argv[0], argv);

            /* Exec failed. */
            exit(-1);
        } case -1: {
            /* Fork failed. */
            DEBUG("failed initial fork (%d)\n", errno);
            free(p);
            return NULL;
        }
    }

    assert(p->pid > 0);
    int status;
    waitpid(p->pid, &status, 0);
    if (WIFEXITED(status)) {
        /* Either ptrace or exec failed in the child. */
        DEBUG("child process (tracee) exited immediately\n");
        free(p);
        return NULL;
    }

    long r = ptrace(PTRACE_SYSCALL, p->pid, NULL, NULL);
    if (r != 0) {
        DEBUG("failed first resume of child process (tracee) (%d)\n", errno);
        free(p);
        return NULL;
    }
    p->running = true;
    p->in_syscall = false;
    return p;
}

static long peekuser(proc_t *proc, long offset) {
    assert(proc != NULL);
    assert(proc->pid > 0);
    return ptrace(PTRACE_PEEKUSER, proc->pid, offset, NULL);
}

#define OFFSET(reg) \
    (__builtin_offsetof(struct user, regs) + \
     __builtin_offsetof(struct user_regs_struct, reg))

static long syscall_number(proc_t *proc) {
    return peekuser(proc, OFFSET(REG_SYSNO));
}

static long syscall_result(proc_t *proc) {
    return peekuser(proc, OFFSET(REG_RESULT));
}

int next_syscall(proc_t *proc, syscall_t *syscall) {
    assert(proc != NULL);
    if (!proc->running) {
        DEBUG("attempt to retrieve a syscall from a stopped process\n");
        return -1;
    }

    assert(proc->pid > 0);
    int status;
    waitpid(proc->pid, &status, 0);
    if (WIFEXITED(status)) {
        proc->running = false;
        proc->exit_status = WEXITSTATUS(status);
        DEBUG("child exited with status %d\n", proc->exit_status);
        return -1;
    }

    syscall->call = syscall_number(proc);
    syscall->enter = !proc->in_syscall;
    assert(!syscall->enter || syscall_result(proc) == -ENOSYS);
    if (!syscall->enter) {
        syscall->result = syscall_result(proc);
    }
    proc->in_syscall = !proc->in_syscall;

    return 0;
}

int acknowledge_syscall(proc_t *proc) {
    assert(proc != NULL);
    assert(proc->pid > 0);
    long r = ptrace(PTRACE_SYSCALL, proc->pid, NULL, NULL);
    if (r != 0) {
        DEBUG("failed to resume process (%d)\n", errno);
    }
    return (int)r;
}

int complete(proc_t *proc) {
    assert(proc != NULL);
    assert(proc->pid > 0);
    if (proc->running) {
        long r = ptrace(PTRACE_CONT, proc->pid, NULL, NULL);
        if (r != 0) {
            DEBUG("failed to resume process (%d)\n", errno);
        }
        int status;
        waitpid(proc->pid, &status, 0);
        assert(WIFEXITED(status));
        proc->running = false;
        proc->exit_status = WEXITSTATUS(status);
    }
    return proc->exit_status;
}

int detach(proc_t *proc) {
    assert(proc != NULL);
    assert(proc->pid > 0);
    long res = ptrace(PTRACE_DETACH, proc->pid, NULL, NULL);
    if (res != 0) {
        DEBUG("failed to detach child (%d)\n", errno);
        return -1;
    }
    free(proc);
    return 0;
}
