#ifndef _XCACHE_DB_H_
#define _XCACHE_DB_H_

#include <sqlite3.h>
#include <sys/types.h>
#include <time.h>

typedef struct {
    sqlite3 *handle;
} db_t;

typedef struct rowset rowset_t;

int db_open(db_t *db, const char *path);
int db_close(db_t *db);

int db_begin(db_t *db);
int db_commit(db_t *db);
int db_rollback(db_t *db);

int db_clear(db_t *db);
int db_select_id(db_t *db, int *id, const char *cwd, const char *command);

int db_insert_id(db_t *db, int *id, const char *cwd, const char *command);
int db_insert_input(db_t *db, int id, const char *filename, time_t timestamp);
int db_insert_output(db_t *db, int id, const char *filename, time_t timestamp,
    mode_t mode, uid_t uid, gid_t gid, const char *contents);

void rowset_discard(rowset_t *rows);

rowset_t *db_select_inputs(db_t *db, int id);
int rowset_next_input(rowset_t *rows, const char **filename, time_t *timestamp);
rowset_t *db_select_outputs(db_t *db, int id);
int rowset_next_output(rowset_t *rows, const char **filename, time_t *timestamp,
    mode_t *mode, uid_t *uid, gid_t *gid, const char **contents);

#endif
