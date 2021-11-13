#pragma once

#include <stdbool.h>
#include <xcache/proc.h>

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

typedef struct xc_trace xc_trace_t;

XCACHE_API int xc_trace_record(xc_trace_t **trace, const xc_proc_t *proc);

XCACHE_API bool xc_trace_valid(xc_trace_t *trace);

/// replay the effects of a previously observed execution
///
/// On success, this function does not return. It mimics the original execution
/// by ending in a call to `exit`.
///
/// \param trace The trace to replay
/// \return An errno on failure
XCACHE_API int xc_trace_replay(const xc_trace_t *trace);
