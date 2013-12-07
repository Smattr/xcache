#ifndef _XCACHE_TRACE_H_
#define _XCACHE_TRACE_H_

#include <stdbool.h>

typedef struct proc proc_t;

typedef struct {
    long call;
    bool enter;
    long result;
} syscall_t;

proc_t *trace(char **argv);

int next_syscall(proc_t *proc, syscall_t *s);
int acknowledge_syscall(proc_t *proc);

int detach(proc_t *proc);

int complete(proc_t *proc);

#endif
