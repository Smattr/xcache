#ifndef _XCACHE_DB_H_
#define _XCACHE_DB_H_

#include "fingerprint.h"
#include <sqlite3.h>
#include <sys/types.h>
#include <time.h>

typedef struct {
    sqlite3 *handle;
} db_t;

int db_open(db_t *db, const char *path);
int db_close(db_t *db);

int db_begin(db_t *db);
int db_commit(db_t *db);
int db_rollback(db_t *db);

int db_clear(db_t *db);
int db_select_id(db_t *db, int *id, const fingerprint_t *fp);

int db_insert_id(db_t *db, int *id, const fingerprint_t *fp);
int db_insert_input(db_t *db, int id, const char *filename, time_t timestamp);
int db_insert_output(db_t *db, int id, const char *filename, time_t timestamp,
    mode_t mode, const char *contents);
int db_insert_env(db_t *db, int id, const char *name, const char *value);

/* Remove a trace entry from the database. Note that this removes child
 * metadata, but does not remove associated cached data itself.
 */
int db_remove_id(db_t *db, int id);

int db_for_inputs(db_t *db, int id,
    int (*cb)(const char *filename, time_t timestamp));
int db_for_outputs(db_t *db, int id,
    int (*cb)(const char *filename, time_t timestamp, mode_t mode,
    const char *contents));
int db_for_env(db_t *db, int id,
        int (*cb)(const char *name, const char *value));

#endif
