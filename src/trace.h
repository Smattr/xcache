#ifndef _XCACHE_TRACE_H_
#define _XCACHE_TRACE_H_

#include "collection/list.h"
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct {
    pid_t pid;
    enum {
        IN_USER,
        SYSENTER,
        IN_KERNEL,
        SYSEXIT,
        TERMINATED,
        FINALISED,
    } state;
} proc_t;

typedef struct {
    unsigned char exit_status;
    proc_t root;

    list_t children;

    char *outfile, *errfile;
    int outfd, errfd;

    int stdout_pipe[2];
    int stderr_pipe[2];
    int msg_pipe[2];
    int sig_pipe[2];

    pthread_t hook;
    dict_t env;

} target_t;

typedef struct {
    proc_t *proc;
    long call;
    bool enter;
    long result;
} syscall_t;

int trace(target_t *t, const char **argv, const char *tracer);

syscall_t *next_syscall(target_t *tracee);
int acknowledge_syscall(syscall_t *syscall);

int delete(target_t *tracee);

int complete(target_t *tracee);

char *syscall_getstring(syscall_t *syscall, int arg);
long syscall_getarg(syscall_t *syscall, int arg);

const char *get_stdout(target_t *tracee);
const char *get_stderr(target_t *tracee);

#endif
