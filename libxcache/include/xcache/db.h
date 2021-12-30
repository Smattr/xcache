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

/// lookup a previously recorded trace
///
/// \param db Database to operate on
/// \param question Process to look for
/// \param answer [out] Found trace on success
/// \return 0 on success or an errno on failure
XCACHE_API int xc_db_load(const xc_db_t *db, const xc_proc_t *question,
                          xc_trace_t **answer);

/// store a recorded trace
///
/// \param db Database to operate on
/// \param proc Process that was recorded
/// \param observation Recorded trace of a process
/// \return 0 on success or an errno on failure
XCACHE_API int xc_db_save(const xc_db_t *db, const xc_proc_t *proc,
                          const xc_trace_t *observation);

/// close an open trace database
///
/// This function is a no-op when called on `NULL`.
///
/// \param db The database to close
XCACHE_API void xc_db_close(xc_db_t *db);

#ifdef __cplusplus
}
#endif
