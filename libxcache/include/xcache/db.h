#pragma once

#include <xcache/db_t.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// open a new or existing Xcache trace database
///
/// \param db [out] Handle to the created database on success
/// \param path Directory in which the database resides
/// \return 0 on success or an errno on failure
XCACHE_API int xc_db_open(xc_db_t **db, const char *path);

XCACHE_API int xc_db_load(xc_db_t *db, const xc_proc_t *question,
                          xc_trace_t **answer);

XCACHE_API int xc_db_save(xc_db_t *db, const xc_trace_t *observation);

/// close an open trace database
///
/// This function is a no-op when called on `NULL`.
///
/// \param db The database to close
XCACHE_API void xc_db_close(xc_db_t *db);

#ifdef __cplusplus
}
#endif
