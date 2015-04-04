#include <assert.h>
#include <fcntl.h>
#include <linux/limits.h>
#include "log.h"
#include "ptrace-wrapper.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "util.h"

long pt_traceme(void) {
    long r = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    if (r != 0)
        return r;
    /* Induce a trap to give our parent (the tracer) an opportunity to attach
     * before we call execve.
     */
    return raise(SIGSTOP);
}

long pt_setoptions(pid_t pid) {
    return ptrace(PTRACE_SETOPTIONS, pid, NULL, 0
            /* trace children */
        |PTRACE_O_TRACEEXEC|PTRACE_O_TRACEFORK|PTRACE_O_TRACEVFORK|PTRACE_O_TRACECLONE
            /* allow us to discriminate between syscalls and signals */
        |PTRACE_O_TRACESYSGOOD);
}

long pt_runtosyscall(pid_t pid) {
    return ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
}

long pt_peekreg(pid_t pid, off_t reg) {
    return ptrace(PTRACE_PEEKUSER, pid, (void*)reg, NULL);
}

char *pt_peekstring(pid_t pid, off_t reg) {
    void *addr = (void*)pt_peekreg(pid, reg);
    if (addr == NULL)
        return NULL;

    /* At this point we could PTRACE_PEEKDATA to read the string, but this only
     * lets us read word-by-word and hence is quite slow. On Linux, it's faster
     * to do this through the /proc file system.
     */

    char *filename = aprintf("/proc/%d/mem", pid);
    if (filename == NULL)
        return NULL;
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        DEBUG("failed to open %s to read string\n", filename);
        free(filename);
        return NULL;
    }

    if (fseek(f, (off_t)addr, SEEK_SET) != 0) {
        DEBUG("failed to seek %s\n", filename);
        fclose(f);
        free(filename);
        return NULL;
    }

    free(filename);

    char *s = malloc(PATH_MAX + 1);
    if (s == NULL) {
        fclose(f);
        return NULL;
    }

    bool r = get(s, PATH_MAX + 1, f);
    fclose(f);
    if (!r) {
        free(s);
        return NULL;
    }

    return s;
}

char *pt_peekfd(pid_t pid, const char *cwd, off_t reg) {
    int fd = (int)pt_peekreg(pid, reg);

    if (fd == AT_FDCWD)
        return strdup(cwd);

    char *fdlink = aprintf("/proc/%d/fd/%d", pid, fd);
    if (fdlink == NULL)
        return NULL;

    char *path = readln(fdlink);
    if (path == NULL)
        DEBUG("Failed to resolve link %s\n", fdlink);
    free(fdlink);

    return path;
}

unsigned long pt_geteventmsg(pid_t pid) {
    unsigned long msg;
    long r = ptrace(PTRACE_GETEVENTMSG, pid, NULL, &msg);
    if (r != 0)
        return (unsigned long)r;
    return msg;
}

long pt_continue(pid_t pid) {
    return ptrace(PTRACE_CONT, pid, NULL, NULL);
}

void pt_passthrough(pid_t pid, int event) {
    assert(WIFSTOPPED(event));
    int sig = WSTOPSIG(event);
    if (sig == SIGSYSCALL)
        pt_runtosyscall(pid);
    else
        ptrace(PTRACE_CONT, pid, NULL, sig);
}

void pt_detach(pid_t pid) {
    kill(pid, SIGSTOP);
    while (true) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFSTOPPED(status)) {
            if (WSTOPSIG(status) == SIGSTOP) {
                long r __attribute__((unused)) = ptrace(PTRACE_DETACH, pid, NULL, SIGCONT);
                assert(r == 0);
                break;
            } else {
                pt_passthrough(pid, status);
            }
        } else {
            assert(WIFSIGNALED(status) || WIFEXITED(status));
            break;
        }
    }
}
