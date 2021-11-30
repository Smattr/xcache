#pragma once

#include <xcache/proc.h>
#include <xcache/trace.h>

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// a database of process traces
typedef struct xc_db xc_db_t;

XCACHE_API int xc_db_open(xc_db_t **db, int flags);

XCACHE_API int xc_db_load(xc_db_t *db, const xc_proc_t *question,
                          xc_trace_t **answer);

XCACHE_API int xc_db_save(xc_db_t *db, const xc_trace_t *observation);

XCACHE_API void xc_db_close(xc_db_t *db);

#ifdef __cplusplus
}
#endif
