/* Logging infrastructure. This is essentially for debugging purposes and
 * probably not relevant to xcache users.
 */

#ifndef _XCACHE_LOG_H_
#define _XCACHE_LOG_H_

#include <stdio.h>
#include <stdlib.h>

/* Log levels */
typedef enum {
    L_QUIET = -1,      /* nothing */
    L_ERROR = 0,       /* errors only */
    L_WARNING = 1,     /* errors and warnings */
    L_INFO = 2,        /* all core notifications */
    L_DEBUG = 3,       /* + tracer debugging output */
    L_IDEBUG = 4,      /* + xcache debugging output */
} verbosity_t;

/* The actual log level */
extern verbosity_t verbosity;

/* File we're logging to */
extern FILE *log_file;

/* Initialise the log */
int log_init(const char *filename);

/* Cleanup and terminate logging */
void log_deinit(void);

extern int log_initialised;

/* Log a message at the given log level */
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

#define ERROR(args...)  LOG(L_ERROR, args)
#define WARN(args...)   LOG(L_WARNING, args)
#define INFO(args...)   LOG(L_INFO, args)
#define DEBUG(args...)  LOG(L_DEBUG, args)
#define IDEBUG(args...) LOG(L_IDEBUG, args)

#endif
