#ifndef _XCACHE_DICT_H_
#define _XCACHE_DICT_H_

/* An implementation of a dictionary. See the leading comment in list.h for why
 * we provide a trivial wrapper of GLib.
 */

#include <stdbool.h>
#include <glib.h>

/* A dictionary. */
typedef struct {
    GHashTable *table;
} dict_t;

/* Construct a new dictionary. Returns 0 on success. */
int dict(dict_t *d);

/* Add a new entry to the dictionary. Replaces any existing entry.
 *
 * d - Dictionary to operate on.
 * key - Key for the entry.
 * value - Value for the entry.
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

/* Deallocate resources associated with a dictionary. It is undefined what will
 * happen if you attempt to use the dictionary after destroying it.
 */
void dict_destroy(dict_t *d);

/* Loop over a dictionary's members, performing a caller-defined action on
 * each.
 */
int dict_foreach(dict_t *d, int (*f)(const char *key, void *value));

#endif
