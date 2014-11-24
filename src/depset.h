#ifndef _XCACHE_DEPSET_H_
#define _XCACHE_DEPSET_H_

#include "collection/dict.h"
#include "collection/set.h"

typedef struct depset depset_t;

depset_t *depset_new(void);
int depset_add_input(depset_t *d, char *filename);
int depset_iter_inputs(depset_t *d, dict_iter_t *i);
int depset_add_output(depset_t *d, char *filename);
int depset_iter_outputs(depset_t *d, set_iter_t *i);
void depset_destroy(depset_t *d);

#endif
