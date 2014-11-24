#ifndef _XCACHE_SET_H_
#define _XCACHE_SET_H_

#include <glib.h>
#include <stdbool.h>

typedef struct {
    GHashTable *table;
} set_t;
int set(set_t *s);
int set_add(set_t *s, const char *item);
bool set_contains(set_t *s, const char *item);
void set_destroy(set_t *s);

/* Iterate over a set performing a given action on each item.
 *
 * s - Set to act on.
 * f - Action to perform. This callback will be invoked once per item with the
 *   item itself as the parameter. Return 0 from this to continue the iteration
 *   or non-zero to abort.
 *
 * Returns 0 on success, or the return value of the callback if the callback
 * causes an early abort.
 */
int set_foreach(set_t *s, int (*f)(const char *value));

#endif
