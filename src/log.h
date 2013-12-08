#ifndef _XCACHE_LOG_H_
#define _XCACHE_LOG_H_

#include <stdio.h>

typedef enum {
    L_QUIET = 0,
    L_ERROR,
    L_WARNING,
    L_INFO,
    L_DEBUG,
} verbosity_t;
extern verbosity_t verbosity;

#define LOG(level, args...) \
    do { \
        if ((level) <= verbosity) { \
            fprintf(stderr, "xcache: " args); \
        } \
    } while (0)

#define ERROR(args...) LOG(L_ERROR, args)
#define WARN(args...)  LOG(L_WARNING, args)
#define INFO(args...)  LOG(L_INFO, args)
#define DEBUG(args...) LOG(L_DEBUG, args)

#endif
