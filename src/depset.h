#ifndef _XCACHE_DEPSET_H_
#define _XCACHE_DEPSET_H_

#include "collection/dict.h"
#include <time.h>

typedef enum {
    XC_NONE,
    XC_INPUT,
    XC_OUTPUT,
    XC_BOTH,
    XC_AMBIGUOUS,
} filetype_t;

typedef struct depset depset_t;

depset_t *depset_new(void);

int depset_add(depset_t *d, char *filename, filetype_t type);
void depset_destroy(depset_t *d);

int depset_foreach(depset_t *d, int (*f)(const char *filename, filetype_t type, time_t mtime));

/* Prepare a dependency set to be serialised to disk. No further elements should
 * be added to the set after this. Returns 0 on success.
 */
int depset_finalise(depset_t *d);

#endif
