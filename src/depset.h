#ifndef _XCACHE_DEPSET_H_
#define _XCACHE_DEPSET_H_

/* Representation of a set of dependencies for a process.
 *
 * Each target we might trace has a collection of file dependencies, which are
 * individually either notionally inputs, outputs or both. When tracing a
 * target, we track this information. The dependency set is precisely the
 * information we examine when re-running a target to determine whether we can
 * retrieved cached results of its previously observed execution.
 */

#include "collection/dict.h"
#include <time.h>

/* The type of a file dependency. */
typedef enum {
    XC_NONE,         /* irrelevant (used as a placeholder) */
    XC_INPUT,        /* the target reads this file */
    XC_OUTPUT,       /* the target writes this file */
    XC_BOTH,         /* the target reads and writes this file */
    XC_AMBIGUOUS,    /* the target does "something" with this file;
                        annoying quirk to handle `stat` */
} filetype_t;

/* A collection of file dependencies. */
typedef struct depset depset_t;

/* Create a new dependency set. Returns NULL on failure. */
depset_t *depset_new(void);

/* Add a file to the dependency set with the given relationship. Returns 0 on
 * success.
 */
int depset_add(depset_t *d, char *filename, filetype_t type);

/* Deallocate resources associated with a dependency set. It is undefined what
 * will happen if you attempt to use a dependency set after you have destroyed
 * it.
 */
void depset_destroy(depset_t *d);

/* Loop over a dependency set, performing a caller-described action on each
 * member.
 */
int depset_foreach(depset_t *d, int (*f)(const char *filename, filetype_t type, time_t mtime));

/* Prepare a dependency set to be serialised to disk. No further elements should
 * be added to the set after this. Returns 0 on success.
 */
int depset_finalise(depset_t *d);

#endif
