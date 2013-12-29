#ifndef _XCACHE_DEPSET_H_
#define _XCACHE_DEPSET_H_

#include "dict.h"
#include "set.h"

typedef struct depset depset_t;

depset_t *depset_new(void);
int depset_add_input(depset_t *oper, char *filename);
iter_t *depset_iter_inputs(depset_t *oper);
int depset_add_output(depset_t *oper, char *filename);
set_iter_t *depset_iter_outputs(depset_t *oper);
void depset_destroy(depset_t *oper);
#endif
