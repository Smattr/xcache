#ifndef _XCACHE_LIST_H_
#define _XCACHE_LIST_H_

/* A wrapper around GLib's lists. Though this does not provide much value for
 * now, the intention is to potentially move away from a dependence on GLib in
 * the future. GLib has pretty unpleasant and inflexible semantics for handling
 * memory exhaustion (unconditional abort()). While this is appropriate for
 * many applications, given that we are wrapping an unknown target, we should
 * really be doing everything possible to not introduce unrecoverable failures.
 */

#include <glib.h>

/* A linked-list. */
typedef struct {
    GSList *head;
    int (*comparator)(void *a, void *b);
} list_t;

/* Construct a linked-list. Returns 0 on success. */
int list(list_t *l, int (*comparator)(void *a, void *b));

/* Add an item to the list. Returns 0 on success */
int list_add(list_t *l, void *value);

/* Return an item from the list given the input item to find. Note that this
 * uses the comparator the caller gave when constructing the list. Returns NULL
 * if the item is not found.
 */
void *list_find(list_t *l, void *key);

/* Find and remove an item from the list. Returns NULL if the item is not
 * found.
 */
void *list_remove(list_t *l, void *key);

/* Perform some action on each item in the list. This is generally used for
 * constructing a loop pattern over a list.
 */
int list_foreach(list_t *l, void (*f)(void *value, void *data), void *data);

/* Deallocate resources associated with a list. It is undefined what will
 * happen if you attempt to use the list after destroying it.
 */
void list_destroy(list_t *l);

#endif
