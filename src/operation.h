#ifndef _XCACHE_OPERATION_H_
#define _XCACHE_OPERATION_H_

typedef struct operation operation_t;

operation_t *operation_new(void);
int operation_add_input(operation_t *oper, char *filename);
int operation_add_output(operation_t *oper, char *filename);
int operation_add_missing(operation_t *oper, char *filename);
void operation_destroy(operation_t *oper);
#endif
