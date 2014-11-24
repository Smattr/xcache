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

int set_foreach(set_t *s, int (*f)(const char *value)) {
    const char *value;
    void *dummy;
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, s->table);
    while (g_hash_table_iter_next(&iter, (gpointer)&value, &dummy)) {
        int ret = f(value);
        if (ret != 0)
            return ret;
    }
    return 0;
}
