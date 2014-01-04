#ifndef _XCACHE_TRACE_H_
#define _XCACHE_TRACE_H_

#include <stdbool.h>
#include <unistd.h>

typedef struct tracee tracee_t;
typedef struct proc proc_t;

typedef struct {
    proc_t *proc;
    long call;
    bool enter;
    long result;
} syscall_t;

tracee_t *trace(const char **argv);

syscall_t *next_syscall(tracee_t *tracee);
int acknowledge_syscall(syscall_t *syscall);

int delete(tracee_t *tracee);

int complete(tracee_t *tracee);

char *syscall_getstring(syscall_t *syscall, int arg);
long syscall_getarg(syscall_t *syscall, int arg);

const char *get_stdout(tracee_t *tracee);
const char *get_stderr(tracee_t *tracee);

#endif
