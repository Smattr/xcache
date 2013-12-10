#ifndef _XCACHE_LOG_H_
#define _XCACHE_LOG_H_

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    L_QUIET = 0,
    L_ERROR,
    L_WARNING,
    L_INFO,
    L_DEBUG,
} verbosity_t;
extern verbosity_t verbosity;

extern FILE *log_file;

int log_init(char *filename);
void log_deinit(void);

extern int log_initialised;

#define LOG(level, args...) \
    do { \
        if (!log_initialised) { \
            log_init(NULL); \
        } \
        if ((level) <= verbosity) { \
            if (log_file == stderr) { \
                fprintf(stderr, "xcache: "); \
            } \
            fprintf(log_file, args); \
        } \
    } while (0)

#define ERROR(args...) LOG(L_ERROR, args)
#define WARN(args...)  LOG(L_WARNING, args)
#define INFO(args...)  LOG(L_INFO, args)
#define DEBUG(args...) LOG(L_DEBUG, args)

#endif
