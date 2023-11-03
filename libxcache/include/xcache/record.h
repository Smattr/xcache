#pragma once

#include <xcache/cmd.h>
#include <xcache/db.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

/** run a command and monitor its behaviour
 *
 * This function `fork`s and spawns background threads. It attempts to join all
 * these subprocesses and threads before returning.
 *
 * \param db Database to record results into
 * \param cmd Command to run
 * \return 0 on success or an errno on failure
 */
XCACHE_API int xc_record(xc_db_t *db, const xc_cmd_t cmd);

#ifdef __cplusplus
}
#endif
