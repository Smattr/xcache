#ifndef _XCACHE_DICT_H_
#define _XCACHE_DICT_H_

typedef struct entry *dict_t;

dict_t *dict_new(void);
int dict_add(dict_t *dict, char *key, void *value, void **oldvalue);
int dict_add_if(dict_t *dict, char *key, void *value, void **oldvalue, void *(*guard)(char *key, void *current));
void *dict_remove(dict_t *dict, char *key);
void *dict_find(dict_t *dict, char *key);
void dict_destroy(dict_t *dict);

#endif
