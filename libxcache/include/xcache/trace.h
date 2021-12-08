#pragma once

#include <stdbool.h>
#include <xcache/db_t.h>
#include <xcache/proc.h>

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// an observed execution of a process and its side effects
typedef struct xc_trace xc_trace_t;

/// run the given process and monitor its behaviour
///
/// This function calls `fork` to run the given process and then monitors it via
/// `seccomp`.
///
/// TODO: explain what happens to the child if we get a failure after it has
/// started running and/or allow the caller to control this.
///
/// \param trace [out] Trace of the process’ inputs and outputs
/// \param proc Process to run
/// \param db Database in which to host output artifacts
/// \return 0 on success or an errno on failure
XCACHE_API int xc_trace_record(xc_trace_t **trace, const xc_proc_t *proc,
                               xc_db_t *db);

/// check whether a trace is not stale
///
/// This evaluates whether conditions have changed since the trace was observed
/// in a way that makes it unsafe to replay this trace.
///
/// \param trace A trace to evaluate
/// \return True iff the trace is usable
XCACHE_API bool xc_trace_is_valid(const xc_trace_t *trace);

/// replay the effects of a previously observed execution
///
/// On success, this function does not return. It mimics the original execution
/// by ending in a call to `exit`.
///
/// \param trace The trace to replay
/// \return An errno on failure
XCACHE_API int xc_trace_replay(const xc_trace_t *trace);

/// clean up and deallocate a previously created trace
///
/// \param trace Trace to deallocate
XCACHE_API void xc_trace_free(xc_trace_t *trace);

#ifdef __cplusplus
}
#endif
