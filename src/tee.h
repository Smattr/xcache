#ifndef _XCACHE_TEE_H_
#define _XCACHE_TEE_H_

#include <pthread.h>

typedef struct tee tee_t;

tee_t *tee_create(int *output);
char *tee_close(tee_t *t);

#endif
