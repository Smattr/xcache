#ifndef _XCACHE_HOOK_H_
#define _XCACHE_HOOK_H_

#include "collection/dict.h"

typedef struct {
    pthread_t thread;
    int sigfd;
} hook_t;

hook_t *hook_create(int input, dict_t *env);

int hook_close(hook_t *h);

#endif
