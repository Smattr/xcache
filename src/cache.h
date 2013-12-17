#ifndef _XCACHE_CACHE_H_
#define _XCACHE_CACHE_H_

#include <stdlib.h>

typedef struct cache cache_t;

#define DATA_LIMIT_DEFAULT ((ssize_t)(100 * 1024 * 1024)) /* bytes */
#define DATA_LIMIT_UNLIMITED SSIZE_MAX
#define DATA_SIZE_UNSET ((ssize_t)-1)
cache_t *cache_open(const char *path, ssize_t size);

int cache_clear(cache_t *cache);

int cache_locate(cache_t *cache, const char **args);

/* Extract the cached outputs associated with a particular identifier and write
 * them out as if the original program had written them. Returns 0 on success,
 * -1 on failure.
 */
int cache_dump(cache_t *cache, int id);

int cache_close(cache_t *cache);

#endif
