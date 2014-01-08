#include <assert.h>
#include "list.h"
#include <stdlib.h>

typedef struct _node {
    void *value;
    struct _node *next;
} node_t;

struct list {
    node_t *head;
    void *(*key)(void *node);
};

list_t *list(void *(*key)(void *node)) {
    list_t *l = (list_t*)calloc(1, sizeof(*l));
    l->key = key;
    return l;
}

int list_add(list_t *l, void *value) {
    assert(l != NULL);
    node_t *n = malloc(sizeof(node_t));
    if (n == NULL)
        return -1;

    n->value = value;
    n->next = l->head;
    l->head = n;
    return 0;
}

void *list_find(list_t *l, void *key) {
    assert(l != NULL);
    for (node_t *p = l->head; p != NULL; p = p->next) {
        if (l->key(p->value) == key) {
            return p->value;
        }
    }
    return NULL;
}

void *list_remove(list_t *l, void *key) {
    assert(l != NULL);
    for (node_t *p = l->head, *q = NULL; p != NULL; q = p, p = p->next) {
        if (l->key(p->value) == key) {
            /* Found it. */
            if (q == NULL)
                l->head = p->next;
            else
                q->next = p->next;
            void *v = p->value;
            free(p);
            return v;
        }
    }
    /* Didn't find it. */
    return NULL;
}

void *list_pop(list_t *l) {
    assert(l != NULL);
    if (l->head == NULL)
        return NULL;

    node_t *p = l->head;
    l->head = l->head->next;
    void *v = p->value;
    free(p);
    return v;
}

void list_destroy(list_t *l) {
    assert(l != NULL);
    free(l);
}
