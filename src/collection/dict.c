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

int dict_add(dict_t *d, const char *key, void *value) {
    char *k = strdup(key);
    if (k == NULL)
        return -1;
    if (value == NULL)
        value = d->value(key);
    g_hash_table_insert(d->table, (gpointer)k, (gpointer)value);
    return 0;
}

void *dict_lookup(dict_t *d, const char *key) {
    return (void*)g_hash_table_lookup(d->table, (gconstpointer)key);
}

bool dict_remove(dict_t *d, const char *key) {
    return (bool)g_hash_table_remove(d->table, (gconstpointer)key);
}

bool dict_contains(dict_t *d, const char *key) {
    return (bool)g_hash_table_contains(d->table, (gconstpointer)key);
}

int dict_foreach(dict_t *d, int (*f)(const char *key, void *value)) {
    const char *key;
    void *value;
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, d->table);
    while (g_hash_table_iter_next(&iter, (gpointer)&key, &value)) {
        int ret = f(key, value);
        if (ret != 0)
            return ret;
    }
    return 0;
}

void dict_destroy(dict_t *d) {
    g_hash_table_remove_all(d->table);
}
