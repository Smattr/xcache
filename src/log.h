/** \file Logging infrastructure.
 *
 * This is essentially for debugging purposes and probably not relevant to
 * xcache users.
 */

#ifndef _XCACHE_LOG_H_
#define _XCACHE_LOG_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** \brief Log levels. */
typedef enum {
    L_QUIET = -1,      /**< nothing                   */
    L_ERROR = 0,       /**< errors only               */
    L_WARNING = 1,     /**< errors and warnings       */
    L_INFO = 2,        /**< all core notifications    */
    L_DEBUG = 3,       /**< + tracer debugging output */
    L_IDEBUG = 4,      /**< + xcache debugging output */
} verbosity_t;

/** \brief The current log level.
 *
 * This is expected to be set/adjusted at program start and never modified from
 * then on.
 */
extern verbosity_t verbosity;

/** \brief Log output.
 *
 * This is never expected to be set or referenced manually by users. It is
 * simply exposed in this header for use in the `LOG` macro.
 */
extern FILE *log_file;

/** \brief Whether the log has been initialised.
 *
 * Again, this is never expected to be modified or referenced manually by
 * users. It is exposed for use in the `LOG` macro.
 */
extern bool log_initialised;

/** \brief Initialise the log.
 *
 * @param filename File to write logging messages to. If `NULL`, the log will
 *   be written to stderr.
 * @return 0 on success, non-zero on failure.
 */
int log_init(const char *filename);

/** \brief Cleanup and terminate logging.
 *
 * Callers should not write to the log after calling this function.
 */
void log_deinit(void);

/** \brief Log a message at the given log level.
 *
 * @param level The log level. This is expected to be a member of
 *   `verbosity_t`.
 * @param args Content to log, given as printf-style arguments.
 */
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

/** \brief Various shortcut macros for logging follow. */

#define ERROR(args...)  LOG(L_ERROR, args)
#define WARN(args...)   LOG(L_WARNING, args)
#define INFO(args...)   LOG(L_INFO, args)
#define DEBUG(args...)  LOG(L_DEBUG, args)
#define IDEBUG(args...) LOG(L_IDEBUG, args)

#endif
