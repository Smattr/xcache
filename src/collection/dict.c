#include <assert.h>
#include "dict.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../util.h"

#define ENTRIES 127

typedef struct _entry {
    char *key;
    void *value;
    struct _entry *next;
} entry_t;

struct dict {
    entry_t **entries;
    size_t size;
    void *(*get_value)(const char *key);
};

dict_t *dict_new(void *(*get_value)(const char *key)) {
    dict_t *d = malloc(sizeof(*d));
    if (d == NULL)
        return NULL;
    d->entries = calloc(ENTRIES, sizeof(*d->entries));
    if (d->entries == NULL) {
        free(d);
        return NULL;
    }
    d->size = ENTRIES;
    d->get_value = get_value;
    return d;
}

static bool contains(dict_t *d, unsigned int index, const char *key) {
    for (entry_t *e = d->entries[index]; e != NULL; e = e->next) {
        if (!strcmp(e->key, key))
            return true;
    }
    return false;
}

int dict_add(dict_t *d, const char *key) {
    unsigned int index = hash(d->size, key);
    if (contains(d, index, key))
        return 0;
    entry_t *e = malloc(sizeof(*e));
    if (e == NULL)
        return -1;
    e->key = strdup(key);
    if (e->key == NULL) {
        free(e);
        return -1;
    }
    e->value = d->get_value(key);
    if (e->value == NULL) {
        free(e->key);
        free(e);
        return -1;
    }
    e->next = d->entries[index];
    d->entries[index] = e;
    return 0;
}

struct dict_iter {
    dict_t *dict;
    unsigned int index;
    entry_t *entry;
};

dict_iter_t *dict_iter(dict_t *d) {
    dict_iter_t *i = malloc(sizeof(*i));
    if (i == NULL)
        return NULL;
    i->dict = d;
    i->index = 0;
    i->entry = d->entries[0];
    return i;
}

void dict_iter_destroy(dict_iter_t *i) {
    free(i);
}

int dict_iter_next(dict_iter_t *i, char **key, void **value) {
    while (i->index < i->dict->size && i->entry == NULL) {
        i->index++;
        i->entry = i->dict->entries[i->index];
    }
    if (i->index == i->dict->size) {
        dict_iter_destroy(i);
        return 1;
    }
    assert(i->entry != NULL);
    *key = i->entry->key;
    *value = i->entry->value;
    i->entry = i->entry->next;
    return 0;
}

void dict_destroy(dict_t *d) {
    for (unsigned int i = 0; i < d->size; i++) {
        for (entry_t *e = d->entries[i]; e != NULL;) {
            entry_t *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(d);
}
