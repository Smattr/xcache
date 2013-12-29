#ifndef _XCACHE_DICT_H_
#define _XCACHE_DICT_H_

typedef struct dict dict_t;

dict_t *dict_new(void *(*get_value)(const char *key));
int dict_add(dict_t *d, const char *key);
void dict_destroy(dict_t *d);

typedef struct dict_iter dict_iter_t;
dict_iter_t *dict_iter(dict_t *d);
int dict_iter_next(dict_iter_t *i, char **key, void **value);

/* Pre-emptively destroy an iterator. This does not need to be called on an
 * iterator that has been exhausted.
 */
void dict_iter_destroy(dict_iter_t *iter);

#endif
