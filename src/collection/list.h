#ifndef _XCACHE_LIST_H_
#define _XCACHE_LIST_H_

typedef struct list list_t;

list_t *list(void *(*key)(void *node));
int list_add(list_t *l, void *value);
void *list_find(list_t *l, void *key);
void *list_remove(list_t *l, void *key);
void *list_pop(list_t *l);
void list_destroy(list_t *l);

#endif
