#ifndef _XCACHE_DICT_H_
#define _XCACHE_DICT_H_

#include <glib.h>

typedef struct {
    GHashTable *table;
    void *(*value)(const char *key);
} dict_t;

int dict(dict_t *d, void *(*get_value)(const char *key));
int dict_add(dict_t *d, const char *key);
void dict_destroy(dict_t *d);

typedef GHashTableIter dict_iter_t;

int dict_iter(dict_t *d, dict_iter_t *i);
int dict_iter_next(dict_iter_t *i, char **key, void **value);

#endif
