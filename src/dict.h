#ifndef _XCACHE_DICT_H_
#define _XCACHE_DICT_H_

#include <stdbool.h>

typedef struct entry *dict_t;

dict_t *dict_new(void);
int dict_add(dict_t *dict, char *key, void *value);
int dict_add_if(dict_t *dict, char *key, void *(*guard)(char *key, void *current));
void *dict_remove(dict_t *dict, char *key);
void *dict_find(dict_t *dict, char *key);
void dict_destroy(dict_t *dict);

typedef struct iter iter_t;
iter_t *dict_iter(dict_t *dict);
bool iter_next(iter_t *iter, char **key, void **value);

/* Pre-emptively destroy an iterator. This does not need to be called on an
 * iterator that has been exhausted.
 */
void iter_destroy(iter_t *iter);

#endif
