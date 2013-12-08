#ifndef _XCACHE_DEPSET_H_
#define _XCACHE_DEPSET_H_

typedef struct depset depset_t;

depset_t *depset_new(void);
int depset_add_input(depset_t *oper, char *filename);
int depset_add_output(depset_t *oper, char *filename);
int depset_add_missing(depset_t *oper, char *filename);
void depset_destroy(depset_t *oper);
#endif
