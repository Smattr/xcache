#pragma once

#include <stdbool.h>
#include <xcache/cmd.h>
#include <xcache/db.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

/// an observed execution of a command and its side effects
typedef struct xc_trace xc_trace_t;

/** search for a previously recorded trace
 *
 * \param db Database in which to search
 * \param query Command to look for
 * \param cb Callback to be invoked for each matching trace
 * \param state Value to pass as second parameter to callback
 * \return 0 on success or an errno on failure
 */
XCACHE_API int xc_trace_find(const xc_db_t *db, const xc_cmd_t query,
                             int (*cb)(const xc_trace_t *trace, void *state),
                             void *state);

/** check a trace is not stale
 *
 * This evaluates whether conditions have changed since the trace was recorded
 * in a way that makes it unsafe to replay this trace.
 *
 * \param trace Trace to evaluate
 * \return True if the trace is usable
 */
XCACHE_API bool xc_trace_is_valid(const xc_trace_t *trace);

/** delete a trace
 *
 * If you have retrieved a trace from the database and found it to be stale,
 * this function can be used to remove it from disk and future search results.
 * The trace handle itself is no longer valid after a call to this function, as
 * if it had been destroyed with a call to `xc_trace_free`.
 *
 * \param trace Trace to remove
 */
XCACHE_API int xc_trace_remove(xc_trace_t *trace);

/** replay the effects of a trace
 *
 * It is assumed the given trace is valid.
 *
 * \param trace Trace to replay
 * \return 0 on success or an errno on failure
 */
XCACHE_API int xc_trace_replay(const xc_trace_t *trace);

/** release a handle to a trace
 *
 * This deallocates backing memory and releases any per-trace locks that may be
 * held.
 *
 * \param trace Trace to deallocate
 */
XCACHE_API void xc_trace_free(xc_trace_t *trace);

#ifdef __cplusplus
}
#endif
