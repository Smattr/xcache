#ifndef _XCACHE_SET_H_
#define _XCACHE_SET_H_

#include <stdbool.h>

typedef struct set set_t;
set_t *set_new(void);
int set_add(set_t *s, const char *item);
bool set_contains(set_t *s, const char *item);
void set_destroy(set_t *s);

typedef struct set_iter set_iter_t;
set_iter_t *set_iter(set_t *s);
void set_iter_destroy(set_iter_t *i);
int set_iter_next(set_iter_t *i, const char **item);

#endif
