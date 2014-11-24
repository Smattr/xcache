#ifndef _XCACHE_DEPSET_H_
#define _XCACHE_DEPSET_H_

#include "collection/dict.h"
#include "collection/set.h"
#include <time.h>

typedef struct depset depset_t;

depset_t *depset_new(void);
int depset_add_input(depset_t *d, char *filename);
int depset_add_output(depset_t *d, char *filename);
void depset_destroy(depset_t *d);

int depset_foreach_input(depset_t *d, int (*f)(const char *filename, time_t mtime));
int depset_foreach_output(depset_t *d, int (*f)(const char *filename));

#endif
