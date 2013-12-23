#include "arch_syscall.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include "file.h"
#include <linux/limits.h>
#include "log.h"
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "trace.h"
#include <unistd.h>

struct proc {
    pid_t pid;
    bool running;
    bool in_syscall;
    unsigned char exit_status;
    char argbuffer[PATH_MAX + 1];
};

proc_t *trace(const char **argv) {
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
            execvp(argv[0], (char**)argv);

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

static long peekuser(proc_t *proc, off_t offset) {
    assert(proc != NULL);
    assert(proc->pid > 0);
    return ptrace(PTRACE_PEEKUSER, proc->pid, (void*)offset, NULL);
}

static char *peekstring(proc_t *proc, off_t reg) {
    char filename[100];

    void *addr = (void*)peekuser(proc, reg);
    if (addr == NULL) {
        DEBUG("attempt to read as a string an argument the target passed as "
            "(nil)\n");
        return NULL;
    }

    /* At this point we could PTRACE_PEEKDATA to read the string, but this only
     * lets us read word-by-word and hence is quite slow. On Linux, it's faster
     * to do this through the /proc file system.
     */

    sprintf(filename, "/proc/%d/mem", proc->pid);
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        DEBUG("failed to open %s to read string\n", filename);
        return NULL;
    }

    if (fseek(f, (off_t)addr, SEEK_SET) != 0) {
        DEBUG("failed to seek %s\n", filename);
        fclose(f);
        return NULL;
    }

    bool r = get(proc->argbuffer, sizeof(proc->argbuffer), f);
    fclose(f);
    return r ? proc->argbuffer : NULL;
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

static long register_offset(int arg) {
    switch (arg) {

        /* Dependending on our architecture, we may only have a subset of the
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

char *syscall_getstring(proc_t *proc, int arg) {
    assert(proc != NULL);
    assert(arg > 0);

    long offset = register_offset(arg);
    if (offset == -1)
        return NULL;

    return peekstring(proc, offset);
}

long syscall_getarg(proc_t *proc, int arg) {
    long offset = register_offset(arg);
    if (offset == -1)
        return -1;
    return peekuser(proc, offset);
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

int delete(proc_t *proc) {
    free(proc);
    return 0;
}
