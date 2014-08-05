#ifndef _XCACHE_CACHE_H_
#define _XCACHE_CACHE_H_

#include "depset.h"
#include <stdlib.h>

typedef struct cache cache_t;

cache_t *cache_open(const char *path);

int cache_clear(cache_t *cache);

int cache_locate(cache_t *cache, const char **args);

/* Extract the cached outputs associated with a particular identifier and write
 * them out as if the original program had written them. Returns 0 on success,
 * -1 on failure.
 */
int cache_dump(cache_t *cache, int id);

int cache_close(cache_t *cache);

int cache_write(cache_t *cache, char *cwd, const char **args, depset_t *depset,
    const char *outfile, const char *errfile);

#endif
