#include <assert.h>
#include <glib.h>
#include "list.h"
#include <stdlib.h>
#include <string.h>

int list(list_t *l, int (*comparator)(void *a, void *b)) {
    memset(l, 0, sizeof(*l));
    l->comparator = comparator;
    return 0;
}

int list_add(list_t *l, void *value) {
    assert(l != NULL);
    l->head = g_slist_prepend(l->head, (gpointer)value);
    return 0;
}

void *list_find(list_t *l, void *key) {
    GSList *found = g_slist_find_custom(l->head, (gconstpointer)key,
        (GCompareFunc)l->comparator);
    if (found == NULL)
        return NULL;
    return found->data;
}

void *list_remove(list_t *l, void *key) {
    void *elem = list_find(l, key);
    if (elem == NULL)
        return NULL;
    l->head = g_slist_remove(l->head, elem);
    return elem;
}

int list_foreach(list_t *l, void (*f)(void *value, void *data), void *data) {
    g_slist_foreach(l->head, f, data);
    return 0;
}

void list_destroy(list_t *l) {
    g_slist_free(l->head);
}
