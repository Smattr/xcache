#include <assert.h>
#include "dict.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ENTRIES 127

typedef struct entry {
    char *key;
    void *value;
    struct entry *next;
} entry_t;

dict_t *dict_new(void) {
    size_t sz = sizeof(entry_t*) * ENTRIES;
    dict_t *d = (dict_t*)malloc(sz);
    if (d == NULL) {
        return NULL;
    }
    memset((void*)d, 0, sz);
    return d;
}

static unsigned int hash(char *s) {
    unsigned int h = 0;
    assert(s != NULL);
    for (; *s != '\0'; s++) {
        h = *s + 63 * h;
    }
    return h % ENTRIES;
}

static entry_t *find(dict_t *dict, char *key, unsigned int index, entry_t **prev) {
    assert(prev != NULL);
    *prev = NULL;

    assert(dict != NULL);
    for (entry_t *p = dict[index]; p != NULL; *prev = p, p = p->next) {
        if (!strcmp(p->key, key)) {
            /* Found it. */
            return p;
        }
    }
    /* Didn't find it. */
    return NULL;
}

int dict_add(dict_t *dict, char *key, void *value, void **oldvalue) {
    unsigned int index = hash(key);
    entry_t *prev;
    entry_t *e = find(dict, key, index, &prev);
    if (e == NULL) {
        e = (entry_t*)malloc(sizeof(entry_t));
        if (e == NULL) {
            return -1;
        }
        e->key = strdup(key);
        if (e->key == NULL) {
            free(e);
            return -1;
        }
        if (oldvalue != NULL) {
            *oldvalue = NULL;
        }
        e->next = dict[index];
        dict[index] = e;
    } else if (oldvalue != NULL) {
        *oldvalue = e->value;
    }
    e->value = value;
    
    return 0;
}

void *dict_remove(dict_t *dict, char *key) {
    unsigned int index = hash(key);
    entry_t *p, *prev;

    p = find(dict, key, index, &prev);
    if (p == NULL) {
        return NULL;
    } else if (prev != NULL) {
        prev->next = p->next;
    } else {
        dict[index] = p->next;
    }
    void *value = p->value;
    free(p->key);
    free(p);
    return value;
}

void *dict_find(dict_t *dict, char *key) {
    unsigned int index = hash(key);
    entry_t *p, *prev;

    p = find(dict, key, index, &prev);
    if (p != NULL) {
        return p->value;
    }
    return NULL;
}

void dict_destroy(dict_t *dict) {
    assert(dict != NULL);
    for (unsigned int i = 0; i < ENTRIES; i++) {
        for (entry_t *e = dict[i]; e != NULL;) {
            entry_t *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(dict);
}
