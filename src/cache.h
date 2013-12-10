#ifndef _XCACHE_CACHE_H_
#define _XCACHE_CACHE_H_

typedef struct cache cache_t;

cache_t *cache_open(const char *path);

int cache_clear(cache_t *cache);

int cache_locate(cache_t *cache, char **args);

int cache_close(cache_t *cache);

#endif
