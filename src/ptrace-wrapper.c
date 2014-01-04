#include <assert.h>
#include "file.h"
#include <linux/limits.h>
#include "log.h"
#include "ptrace-wrapper.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

long pt_traceme(void) {
    return ptrace(PTRACE_TRACEME, 0, NULL, NULL);
}

long pt_tracechildren(pid_t pid) {
    return ptrace(PTRACE_SETOPTIONS, pid, NULL,
        PTRACE_O_TRACEFORK|PTRACE_O_TRACEVFORK|PTRACE_O_TRACECLONE);
}

long pt_runtosyscall(pid_t pid) {
    return ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
}

long pt_peekreg(pid_t pid, off_t reg) {
    return ptrace(PTRACE_PEEKUSER, pid, (void*)reg, NULL);
}

char *pt_peekstring(pid_t pid, off_t reg) {
    char filename[100];

    void *addr = (void*)pt_peekreg(pid, reg);
    if (addr == NULL)
        return NULL;

    /* At this point we could PTRACE_PEEKDATA to read the string, but this only
     * lets us read word-by-word and hence is quite slow. On Linux, it's faster
     * to do this through the /proc file system.
     */

    sprintf(filename, "/proc/%d/mem", pid);
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

    char *s = (char*)malloc(PATH_MAX + 1);
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

long pt_continue(pid_t pid) {
    return ptrace(PTRACE_CONT, pid, NULL, NULL);
}

void pt_passthrough(pid_t pid, int event) {
    assert(WIFSTOPPED(event));
    int sig = WSTOPSIG(event);
    if (sig == SIGTRAP)
        pt_continue(pid);
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
                long r = ptrace(PTRACE_DETACH, pid, NULL, SIGCONT);
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
