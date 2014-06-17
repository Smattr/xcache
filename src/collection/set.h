#ifndef _XCACHE_SET_H_
#define _XCACHE_SET_H_

#include <glib.h>
#include <stdbool.h>

typedef struct {
    GHashTable *table;
} set_t;
int set(set_t *s);
int set_add(set_t *s, const char *item);
bool set_contains(set_t *s, const char *item);
void set_destroy(set_t *s);

typedef GHashTableIter set_iter_t;
int set_iter(set_t *s, set_iter_t *i);
int set_iter_next(set_iter_t *i, const char **item);

#endif
