#ifndef _XCACHE_LIST_H_
#define _XCACHE_LIST_H_

#include <glib.h>

typedef struct {
    GSList *head;
    int (*comparator)(void *a, void *b);
} list_t;

int list(list_t *l, int (*comparator)(void *a, void *b));
int list_add(list_t *l, void *value);
void *list_find(list_t *l, void *key);
void *list_remove(list_t *l, void *key);
int list_foreach(list_t *l, void (*f)(void *value, void *data), void *data);
void list_destroy(list_t *l);

#endif
