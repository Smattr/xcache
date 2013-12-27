#include <assert.h>
#include "db.h"
#include "queries.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <time.h>

struct db {
    sqlite3 *handle;
};

int db_exec(db_t *db, const char *query) {
    return sqlite3_exec(db->handle, query, NULL, NULL, NULL) != SQLITE_OK;
}

db_t *db_open(const char *path) {
    db_t *d = (db_t*)malloc(sizeof(*d));
    if (d == NULL)
        return NULL;

    int r = sqlite3_open(path, &d->handle);
    if (r != SQLITE_OK) {
        free(d);
        return NULL;
    }

    if (db_exec(d, query_create) != 0) {
        db_close(d);
        free(d);
        return NULL;
    }

    return d;
}

int db_begin(db_t *db) {
    return db_exec(db, "begin transaction");
}
int db_commit(db_t *db) {
    return db_exec(db, "commit transaction");
}
int db_rollback(db_t *db) {
    return db_exec(db, "rollback transaction");
}

int db_clear(db_t *db) {
    return db_exec(db, query_truncate);
}

int db_close(db_t *db) {
    if (sqlite3_close(db->handle) != SQLITE_OK)
        return -1;
    free(db);
    return 0;
}

static int prepare(db_t *db, sqlite3_stmt **s, const char *query) {
    return sqlite3_prepare_v2(db->handle, query, -1, s, NULL);
}

static int bind_text(sqlite3_stmt *s, const char *param, const char *value) {
    int index = sqlite3_bind_parameter_index(s, param);
    if (index == 0)
        return !SQLITE_OK;

    return sqlite3_bind_text(s, index, value, -1, SQLITE_STATIC);
}

static int bind_int(sqlite3_stmt *s, const char *param, int value) {
    int index = sqlite3_bind_parameter_index(s, param);
    if (index == 0)
        return !SQLITE_OK;

    return sqlite3_bind_int(s, index, value);
}

static int bind_time(sqlite3_stmt *s, const char *param, time_t value) {
    int index = sqlite3_bind_parameter_index(s, param);
    if (index == 0)
        return !SQLITE_OK;

    return sqlite3_bind_int64(s, index, (sqlite3_int64)value);
}

static int bind_mode(sqlite3_stmt *s, const char *param, mode_t value) {
    int index = sqlite3_bind_parameter_index(s, param);
    if (index == 0)
        return !SQLITE_OK;

    return sqlite3_bind_int(s, index, (int)value);
}

static int bind_uid(sqlite3_stmt *s, const char *param, uid_t value) {
    int index = sqlite3_bind_parameter_index(s, param);
    if (index == 0)
        return !SQLITE_OK;

    return sqlite3_bind_int(s, index, (int)value);
}

static int bind_gid(sqlite3_stmt *s, const char *param, gid_t value) {
    int index = sqlite3_bind_parameter_index(s, param);
    if (index == 0)
        return !SQLITE_OK;

    return sqlite3_bind_int(s, index, (int)value);
}

int db_select_id(db_t *db, int *id, const char *cwd, const char *command) {
    sqlite3_stmt *s;
    if (prepare(db, &s, query_getid) != SQLITE_OK)
        return -1;

    int result = -1;

    if (bind_text(s, "@cwd", cwd) != SQLITE_OK ||
            bind_text(s, "@command", command) != SQLITE_OK)
        goto fail;

    if (sqlite3_step(s) != SQLITE_ROW)
        goto fail;

    assert(sqlite3_column_count(s) == 1);
    assert(sqlite3_column_type(s, 0) == SQLITE_INTEGER);
    *id = sqlite3_column_int(s, 0);

    assert(sqlite3_step(s) == SQLITE_DONE);

    result = 0;

fail:
    sqlite3_finalize(s);

    return result;
}

int db_insert_id(db_t *db, int *id, const char *cwd, const char *command) {
    sqlite3_stmt *s;
    if (prepare(db, &s, query_addoperation) != SQLITE_OK)
        return -1;

    int result = -1;

    if (bind_text(s, "@cwd", cwd) != SQLITE_OK ||
            bind_text(s, "@command", command) != SQLITE_OK)
        goto fail;

    if (sqlite3_step(s) != SQLITE_DONE)
        goto fail;

    result = db_select_id(db, id, cwd, command);
    assert(result == 0);

fail:
    sqlite3_finalize(s);

    return result;
}

int db_insert_input(db_t *db, int id, const char *filename, time_t timestamp) {
    sqlite3_stmt *s;
    if (prepare(db, &s, query_addinput) != SQLITE_OK)
        return -1;

    int result = -1;

    if (bind_int(s, "@fk_operation", id) != SQLITE_OK ||
            bind_text(s, "@filename", filename) != SQLITE_OK ||
            bind_time(s, "@timestamp", timestamp) != SQLITE_OK)
        goto fail;

    if (sqlite3_step(s) != SQLITE_DONE)
        goto fail;

    result = 0;

fail:
    sqlite3_finalize(s);

    return result;
}

int db_insert_output(db_t *db, int id, const char *filename, time_t timestamp,
        mode_t mode, uid_t uid, gid_t gid, const char *contents) {
    sqlite3_stmt *s;
    if (prepare(db, &s, query_addoutput) != SQLITE_OK)
        return -1;

    int result = -1;

    if (bind_int(s, "@fk_operation", id) != SQLITE_OK ||
            bind_text(s, "@filename", filename) != SQLITE_OK ||
            bind_time(s, "@timestamp", timestamp) != SQLITE_OK ||
            bind_mode(s, "@mode", mode) != SQLITE_OK ||
            bind_uid(s, "@uid", uid) != SQLITE_OK ||
            bind_gid(s, "@gid", gid) != SQLITE_OK ||
            bind_text(s, "@contents", contents) != SQLITE_OK)
        goto fail;

    if (sqlite3_step(s) != SQLITE_DONE)
        goto fail;

    result = 0;

fail:
    sqlite3_finalize(s);

    return result;
}

struct rowset {
    sqlite3_stmt *s;
};

void rowset_discard(rowset_t *rows) {
    sqlite3_finalize(rows->s);
    free(rows);
}

rowset_t *db_select_inputs(db_t *db, int id) {
    rowset_t *r = (rowset_t*)malloc(sizeof(*r));
    if (r == NULL)
        return NULL;

    if (prepare(db, &r->s, query_getinputs) != SQLITE_OK) {
        free(r);
        return NULL;
    }

    if (bind_int(r->s, "@fk_operation", id) != SQLITE_OK) {
        rowset_discard(r);
        return NULL;
    }

    return r;
}

int rowset_next_input(rowset_t *rows, const char **filename, time_t *timestamp) {
    switch (sqlite3_step(rows->s)) {
        case SQLITE_DONE:
            rowset_discard(rows);
            return 1;

        case SQLITE_ROW:
            assert(sqlite3_column_count(rows->s) == 2);
            assert(sqlite3_column_type(rows->s, 0) == SQLITE_TEXT);
            *filename = (const char*)sqlite3_column_text(rows->s, 0);
            assert(*filename != NULL);
            assert(sqlite3_column_type(rows->s, 1) == SQLITE_INTEGER);
            *timestamp = (time_t)sqlite3_column_int64(rows->s, 1);
            return 0;

        default:
            rowset_discard(rows);
            return -1;
    }
}

rowset_t *db_select_outputs(db_t *db, int id) {
    rowset_t *r = (rowset_t*)malloc(sizeof(*r));
    if (r == NULL)
        return NULL;

    if (prepare(db, &r->s, query_getoutputs) != SQLITE_OK) {
        free(r);
        return NULL;
    }

    if (bind_int(r->s, "@fk_operation", id) != SQLITE_OK) {
        rowset_discard(r);
        return NULL;
    }

    return r;
}

int rowset_next_output(rowset_t *rows, const char **filename, time_t *timestamp,
        mode_t *mode, uid_t *uid, gid_t *gid, const char **contents) {
    switch (sqlite3_step(rows->s)) {
        case SQLITE_DONE:
            rowset_discard(rows);
            return 1;

        case SQLITE_ROW:
            assert(sqlite3_column_count(rows->s) == 6);
            assert(sqlite3_column_type(rows->s, 0) == SQLITE_TEXT);
            *filename = (const char*)sqlite3_column_text(rows->s, 0);
            assert(*filename != NULL);
            assert(sqlite3_column_type(rows->s, 1) == SQLITE_INTEGER);
            *timestamp = (time_t)sqlite3_column_int64(rows->s, 1);
            assert(sqlite3_column_type(rows->s, 2) == SQLITE_INTEGER);
            *mode = (mode_t)sqlite3_column_int(rows->s, 2);
            assert(sqlite3_column_type(rows->s, 3) == SQLITE_INTEGER);
            *uid = (uid_t)sqlite3_column_int(rows->s, 3);
            assert(sqlite3_column_type(rows->s, 4) == SQLITE_INTEGER);
            *gid = (gid_t)sqlite3_column_int(rows->s, 4);
            assert(sqlite3_column_type(rows->s, 5) == SQLITE_TEXT);
            *contents = (const char*)sqlite3_column_text(rows->s, 5);
            return 0;

        default:
            rowset_discard(rows);
            return -1;
    }
}

