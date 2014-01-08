#ifndef _XCACHE_DEPSET_H_
#define _XCACHE_DEPSET_H_

#include "collection/dict.h"
#include "set.h"

typedef struct depset depset_t;

depset_t *depset_new(void);
int depset_add_input(depset_t *d, char *filename);
dict_iter_t *depset_iter_inputs(depset_t *d);
int depset_add_output(depset_t *d, char *filename);
set_iter_t *depset_iter_outputs(depset_t *d);
void depset_destroy(depset_t *d);
#endif
