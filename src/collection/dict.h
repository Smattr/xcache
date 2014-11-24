#ifndef _XCACHE_DICT_H_
#define _XCACHE_DICT_H_

#include <stdbool.h>
#include <glib.h>

typedef struct {
    GHashTable *table;
    void *(*value)(const char *key);
} dict_t;

int dict(dict_t *d, void *(*get_value)(const char *key));

/* Add a new entry to the dictionary. Replaces any existing entry.
 *
 * d - Dictionary to operate on.
 * key - Key for the entry.
 * value - Value for the entry. If you pass NULL, a value will be obtained from
 *   the value function of the dictionary itself.
 *
 * Returns 0 on success, non-zero on failure.
 */
int dict_add(dict_t *d, const char *key, void *value);

/* Lookup an existing item in the dictionary.
 *
 * d - Dictionary to operate on.
 * key - Key to look for.
 *
 * Returns the value for the key or NULL if not found.
 */
void *dict_lookup(dict_t *d, const char *key);

/* Remove an entry from the dictionary.
 *
 * d - Dictionary to operate on.
 * key - Key to remove.
 *
 * Returns true if the item was found and removed, false if it was not found.
 */
bool dict_remove(dict_t *d, const char *key);

/* Whether a dictionary already contains an entry for the given key. */
bool dict_contains(dict_t *d, const char *key);

void dict_destroy(dict_t *d);

typedef GHashTableIter dict_iter_t;

int dict_iter(dict_t *d, dict_iter_t *i);
int dict_iter_next(dict_iter_t *i, char **key, void **value);

#endif
