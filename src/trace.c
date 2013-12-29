#include "arch_syscall.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include "file.h"
#include <linux/limits.h>
#include "log.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tee.h"
#include "trace.h"
#include <unistd.h>

struct proc {
    pid_t pid;
    unsigned char exit_status;
    char argbuffer[PATH_MAX + 1];

    enum {
        SETUP = 0,
        RUNNING_IN_USER,
        RUNNING_IN_KERNEL,
        TERMINATED,
        DETACHED,
        FINALISED,
    } state;

    tee_t *out, *err;
    char *outfile, *errfile;
};

proc_t *trace(const char **argv) {
    proc_t *p = (proc_t*)calloc(1, sizeof(proc_t));
    if (p == NULL)
        return NULL;

    int stdout2 = STDOUT_FILENO;
    p->out = tee_create(&stdout2);
    if (p->out == NULL)
        goto fail;

    int stderr2 = STDERR_FILENO;
    p->err = tee_create(&stderr2);
    if (p->err == NULL)
        goto fail;

    p->pid = fork();
    switch (p->pid) {

        case 0: {
            /* We are the child. */
            if (dup2(stdout2, STDOUT_FILENO) == -1 ||
                    dup2(stderr2, STDERR_FILENO) == -1)
                exit(-1);

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
            goto fail;
        }
    }

    assert(p->pid > 0);
    int status;
    waitpid(p->pid, &status, 0);
    if (WIFEXITED(status)) {
        /* Either ptrace or exec failed in the child. */
        DEBUG("child process (tracee) exited immediately\n");
        goto fail;
    }

    long r = ptrace(PTRACE_SYSCALL, p->pid, NULL, NULL);
    if (r != 0) {
        DEBUG("failed first resume of child process (tracee) (%d)\n", errno);
        goto fail;
    }
    p->state = RUNNING_IN_USER;
    return p;

fail:
    if (p->err != NULL) {
        char *t = tee_close(p->err);
        if (t != NULL)
            unlink(t);
        free(t);
    }
    if (p->out != NULL) {
        char *t = tee_close(p->out);
        if (t != NULL)
            unlink(t);
        free(t);
    }
    free(p);
    return NULL;
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
    assert(proc->state == RUNNING_IN_USER || proc->state == RUNNING_IN_KERNEL);

    long offset = register_offset(arg);
    if (offset == -1)
        return NULL;

    return peekstring(proc, offset);
}

long syscall_getarg(proc_t *proc, int arg) {
    assert(proc->state == RUNNING_IN_USER || proc->state == RUNNING_IN_KERNEL);
    long offset = register_offset(arg);
    if (offset == -1)
        return -1;
    return peekuser(proc, offset);
}

int next_syscall(proc_t *proc, syscall_t *syscall) {
    assert(proc != NULL);
    if (proc->state != RUNNING_IN_USER && proc->state != RUNNING_IN_KERNEL) {
        DEBUG("attempt to retrieve a syscall from a stopped process\n");
        return -1;
    }

    assert(proc->pid > 0);
    int status;
    waitpid(proc->pid, &status, 0);
    if (WIFEXITED(status)) {
        proc->state = TERMINATED;
        proc->exit_status = WEXITSTATUS(status);
        DEBUG("child exited with status %d\n", proc->exit_status);
        return -1;
    }

    syscall->call = syscall_number(proc);
    syscall->enter = (proc->state == RUNNING_IN_USER);
    assert(!syscall->enter || syscall_result(proc) == -ENOSYS);
    if (!syscall->enter) {
        syscall->result = syscall_result(proc);
    }
    if (proc->state == RUNNING_IN_USER) {
        proc->state = RUNNING_IN_KERNEL;
    } else {
        assert(proc->state == RUNNING_IN_KERNEL);
        proc->state = RUNNING_IN_USER;
    }

    return 0;
}

int acknowledge_syscall(proc_t *proc) {
    assert(proc != NULL);
    assert(proc->pid > 0);
    assert(proc->state == RUNNING_IN_USER || proc->state == RUNNING_IN_KERNEL);
    long r = ptrace(PTRACE_SYSCALL, proc->pid, NULL, NULL);
    if (r != 0) {
        DEBUG("failed to resume process (%d)\n", errno);
    }
    return (int)r;
}

int complete(proc_t *proc) {
    assert(proc != NULL);
    assert(proc->pid > 0);
    if (proc->state == RUNNING_IN_USER || proc->state == RUNNING_IN_KERNEL) {
        long r = ptrace(PTRACE_DETACH, proc->pid, NULL, NULL);
        if (r != 0)
            DEBUG("failed to detach process (%d)\n", errno);
        int status;
        waitpid(proc->pid, &status, 0);
        assert(WIFEXITED(status));
        proc->state = TERMINATED;
        proc->exit_status = WEXITSTATUS(status);
    }
    assert(proc->outfile == NULL);
    proc->outfile = tee_close(proc->out);
    assert(proc->errfile == NULL);
    proc->errfile = tee_close(proc->err);
    proc->state = FINALISED;
    return proc->exit_status;
}

const char *get_stdout(proc_t *proc) {
    assert(proc->state == FINALISED);
    return proc->outfile;
}

const char *get_stderr(proc_t *proc) {
    assert(proc->state == FINALISED);
    return proc->errfile;
}

int delete(proc_t *proc) {
    if (proc->errfile != NULL)
        free(proc->errfile);
    if (proc->outfile != NULL)
        free(proc->outfile);
    free(proc);
    return 0;
}
