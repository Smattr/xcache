#include <assert.h>
#include "dict.h"
#include <glib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../util.h"

int dict(dict_t *d, void *(*value)(const char *key)) {
    d->table = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    d->value = value;
    return 0;
}

int dict_add(dict_t *d, const char *key) {
    char *k = strdup(key);
    if (k == NULL)
        return -1;
    void *value = d->value(key);
    g_hash_table_insert(d->table, (gpointer)k, (gpointer)value);
    return 0;
}

bool dict_contains(dict_t *d, const char *key) {
    return (bool)g_hash_table_contains(d->table, (gconstpointer)key);
}

int dict_iter(dict_t *d, dict_iter_t *i) {
    g_hash_table_iter_init(i, d->table);
    return 0;
}

int dict_iter_next(dict_iter_t *i, char **key, void **value) {
    return g_hash_table_iter_next(i, (gpointer)key, (gpointer)value);
}

void dict_destroy(dict_t *d) {
    g_hash_table_remove_all(d->table);
}
