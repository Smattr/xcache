#ifndef _XCACHE_CONSTANTS_H_
#define _XCACHE_CONSTANTS_H_

#include <stdlib.h>
#include <time.h>

/* Some time_t values that we're going to treat as having special semantics. */
enum {
    UNSET = (time_t)NULL,    /* we don't know the timestamp of this item */
    MISSING = (time_t)(-2),  /* this file does not exist */
};

#endif
