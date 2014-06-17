#include <assert.h>
#include <glib.h>
#include "set.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../util.h"

int set(set_t *s) {
    s->table = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    return 0;
}

int set_add(set_t *s, const char *item) {
    char *i = strdup(item);
    if (i == NULL)
        return -1;
    g_hash_table_add(s->table, (gpointer)i);
    return 0;
}

bool set_contains(set_t *s, const char *item) {
    return g_hash_table_contains(s->table, (gconstpointer)item);
}

void set_destroy(set_t *s) {
    g_hash_table_remove_all(s->table);
}

int set_iter(set_t *s, set_iter_t *i) {
    g_hash_table_iter_init(i, s->table);
    return 0;
}

int set_iter_next(set_iter_t *i, const char **item) {
    return (int)g_hash_table_iter_next(i, (void**)item, NULL);
}
