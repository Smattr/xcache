#ifndef _XCACHE_DEBUG_H_
#define _XCACHE_DEBUG_H_

#include <stdbool.h>
#include <stdio.h>

extern bool debug;

#define DEBUG(args...) \
    do { \
        if (debug) { \
            fprintf(stderr, "xcache: " args); \
        } \
    } while (0)

#endif
