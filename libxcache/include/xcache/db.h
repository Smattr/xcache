#pragma once

#include <xcache/cmd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

/// a database of command traces
typedef struct xc_db xc_db_t;

/** open a new or existing trace database
 *
 * \param path Directory in which the database resides
 * \param db [out] Handle to the opened database on success
 * \return 0 on success or an errno on failure
 */
XCACHE_API int xc_db_open(const char *path, xc_db_t **db);

/** run a command and monitor its behaviour
 *
 * \param db Database to record results into
 * \param cmd Command to run
 * \return 0 on success or an errno on failure
 */
XCACHE_API int xc_record(xc_db_t *db, const xc_cmd_t cmd);

/** close an open trace database
 *
 * Closing `NULL` is a no-op.
 *
 * \param db Database handle to close
 */
XCACHE_API void xc_db_close(xc_db_t *db);

#ifdef __cplusplus
}
#endif
