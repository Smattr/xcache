#include <assert.h>
#include "set.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

#define ENTRIES 127

typedef struct _entry {
    char *item;
    struct _entry *next;
} entry_t;

struct set {
    entry_t **entries;
    size_t size;
};

set_t *set_new(void) {
    set_t *s = (set_t*)malloc(sizeof(set_t));
    if (s == NULL)
        return NULL;
    s->entries = (entry_t**)calloc(ENTRIES, sizeof(entry_t*));
    if (s->entries == NULL) {
        free(s);
        return NULL;
    }
    s->size = ENTRIES;
    return s;
}

static bool contains(set_t *s, unsigned int index, const char *item) {
    for (entry_t *e = s->entries[index]; e != NULL; e = e->next) {
        if (!strcmp(e->item, item))
            return true;
    }
    return false;
}

int set_add(set_t *s, const char *item) {
    assert(s != NULL);
    assert(item != NULL);

    unsigned int index = hash(s->size, item);
    if (!contains(s, index, item)) {
        entry_t *e = (entry_t*)malloc(sizeof(entry_t));
        if (e == NULL)
            return -1;
        e->item = strdup(item);
        if (e->item == NULL) {
            free(e);
            return -1;
        }
        e->next = s->entries[index];
        s->entries[index] = e;
    }
    return 0;
}

bool set_contains(set_t *s, const char *item) {
    unsigned int h = hash(s->size, item);
    return contains(s, h, item);
}

void set_destroy(set_t *s) {
    for (unsigned int i = 0; i < s->size; i++) {
        for (entry_t *e = s->entries[i], *p; e != NULL;) {
            free(e->item);
            p = e;
            e = e->next;
            free(p);
        }
    }
    free(s);
}

struct set_iter {
    set_t *set;
    unsigned int index;
    entry_t *entry;
};

set_iter_t *set_iter(set_t *s) {
    set_iter_t *i = (set_iter_t*)malloc(sizeof(set_iter_t));
    if (i == NULL)
        return NULL;
    i->set = s;
    i->index = 0;
    i->entry = s->entries[0];
    return i;
}

void set_iter_destroy(set_iter_t *i) {
    free(i);
}

int set_iter_next(set_iter_t *i, const char **item) {
    while (i->index < i->set->size && i->entry == NULL) {
        i->index++;
        i->entry = i->set->entries[i->index];
    }
    if (i->index == i->set->size) {
        /* We've exhausted the iterator. */
        set_iter_destroy(i);
        return 1;
    }
    assert(i->entry != NULL);
    *item = i->entry->item;
    i->entry = i->entry->next;
    return 0;
}
