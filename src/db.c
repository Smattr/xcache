#include <assert.h>
#include "db.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Wrapper around sqlite3_prepare_v2. */
static int prepare(db_t *db, sqlite3_stmt **s, const char *query) {
    return sqlite3_prepare_v2(db->handle, query, -1, s, NULL);
}

/* Wrapper around sqlite3_exec. */
static int exec(db_t *db, const char *query) {
    return sqlite3_exec(db->handle, query, NULL, NULL, NULL);
}

int db_open(db_t *db, const char *path) {
    int r = sqlite3_open(path, &db->handle);
    if (r != SQLITE_OK)
        return -1;

    char *query =
        "create table if not exists operation ("
        "    id integer primary key autoincrement,"
        "    cwd text not null,"
        "    command text not null);"

        "create table if not exists input ("
        "    fk_operation integer references operation(id),"
        "    filename text not null,"
        "    timestamp integer not null);"

        "create table if not exists output ("
        "    fk_operation integer references operation(id),"
        "    filename text not null,"
        "    timestamp integer not null,"
        "    mode integer not null,"
        "    uid integer not null,"
        "    gid integer not null,"
        "    contents text not null);";
    if (exec(db, query) != 0) {
        db_close(db);
        return -1;
    }

    return 0;
}

int db_begin(db_t *db) {
    return exec(db, "begin transaction");
}
int db_commit(db_t *db) {
    return exec(db, "commit transaction");
}
int db_rollback(db_t *db) {
    return exec(db, "rollback transaction");
}

int db_clear(db_t *db) {
    return exec(db,
        "delete from input;"
        "delete from output;"
        "delete from operation;");
}

int db_close(db_t *db) {
    if (sqlite3_close(db->handle) != SQLITE_OK)
        return -1;
    return 0;
}

#define X(c_type, sql_type) \
    static int bind_##c_type(sqlite3_stmt *s, const char *param, const c_type value) { \
        int index = sqlite3_bind_parameter_index(s, param); \
        if (index == 0) \
            return !SQLITE_OK; \
        if (!strcmp(#sql_type, "int64")) \
            return sqlite3_bind_##sql_type(s, index, (sqlite3_int64)value); \
        else \
            return sqlite3_bind_##sql_type(s, index, value); \
    }
#include "sql-type-mapping.h"
#undef X

static int bind_text(sqlite3_stmt *s, const char *param, const char *value) {
    int index = sqlite3_bind_parameter_index(s, param);
    if (index == 0)
        return !SQLITE_OK;

    return sqlite3_bind_text(s, index, value, -1, SQLITE_STATIC);
}

#define X(c_type, sql_type) \
    static c_type column_##c_type(sqlite3_stmt *s, int index) { \
        assert(sqlite3_column_type(s, index) == SQLITE_INTEGER); \
        return (c_type)sqlite3_column_##sql_type(s, index); \
    }
#include "sql-type-mapping.h"
#undef X

static const char *column_text(sqlite3_stmt *s, int index) {
    assert(sqlite3_column_type(s, index) == SQLITE_TEXT);
    return (const char*)sqlite3_column_text(s, index);
}

int db_select_id(db_t *db, int *id, const char *cwd, const char *command) {
    sqlite3_stmt *s;
    char *getid = "select id from operation where cwd = @cwd and command = @command;";
    if (prepare(db, &s, getid) != SQLITE_OK)
        return -1;

    int result = -1;

    if (bind_text(s, "@cwd", cwd) != SQLITE_OK ||
            bind_text(s, "@command", command) != SQLITE_OK)
        goto fail;

    if (sqlite3_step(s) != SQLITE_ROW)
        goto fail;

    assert(sqlite3_column_count(s) == 1);
    *id = column_int(s, 0);

    assert(sqlite3_step(s) == SQLITE_DONE);

    result = 0;

fail:
    sqlite3_finalize(s);

    return result;
}

int db_remove_id(db_t *db, int id) {
    sqlite3_stmt *s = NULL;

    int result = -1;

    char *deleteoutput = "delete from output where fk_operation = @id;";

    if (prepare(db, &s, deleteoutput) != SQLITE_OK)
        goto fail;
    if (bind_int(s, "@fk_operation", id) != SQLITE_OK)
        goto fail;
    if (sqlite3_step(s) != SQLITE_DONE)
        goto fail;
    sqlite3_finalize(s);
    s = NULL;

    char *deleteinput = "delete from intput where fk_operation = @id;";

    if (prepare(db, &s, deleteinput) != SQLITE_OK)
        goto fail;
    if (bind_int(s, "@fk_operation", id) != SQLITE_OK)
        goto fail;
    if (sqlite3_step(s) != SQLITE_DONE)
        goto fail;
    sqlite3_finalize(s);
    s = NULL;

    char *deleteoperation = "delete from operation where id = @id;";
    if (prepare(db, &s, deleteoperation) != SQLITE_OK)
        goto fail;
    if (bind_int(s, "@id", id) != SQLITE_OK)
        goto fail;
    if (sqlite3_step(s) != SQLITE_DONE)
        goto fail;

    result = 0;

fail:
    if (s != NULL)
        sqlite3_finalize(s);

    return result;
}

int db_insert_id(db_t *db, int *id, const char *cwd, const char *command) {
    sqlite3_stmt *s;
    char *add = "insert into operation (cwd, command) values (@cwd, @command);";
    if (prepare(db, &s, add) != SQLITE_OK)
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
    char *add = "insert into input (fk_operation, filename, timestamp) values "
        "(@fk_operation, @filename, @timestamp);";
    if (prepare(db, &s, add) != SQLITE_OK)
        return -1;

    int result = -1;

    if (bind_int(s, "@fk_operation", id) != SQLITE_OK ||
            bind_text(s, "@filename", filename) != SQLITE_OK ||
            bind_time_t(s, "@timestamp", timestamp) != SQLITE_OK)
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
    char *add = "insert into output (fk_operation, filename, timestamp, mode, "
        "uid, gid, contents) values (@fk_operation, @filename, @timestamp, "
        "@mode, @uid, @gid, @contents);";
    if (prepare(db, &s, add) != SQLITE_OK)
        return -1;

    int result = -1;

    if (bind_int(s, "@fk_operation", id) != SQLITE_OK ||
            bind_text(s, "@filename", filename) != SQLITE_OK ||
            bind_time_t(s, "@timestamp", timestamp) != SQLITE_OK ||
            bind_mode_t(s, "@mode", mode) != SQLITE_OK ||
            bind_uid_t(s, "@uid", uid) != SQLITE_OK ||
            bind_gid_t(s, "@gid", gid) != SQLITE_OK ||
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

    char *getinputs = "select filename, timestamp from input where "
        "fk_operation = @fk_operation;";
    if (prepare(db, &r->s, getinputs) != SQLITE_OK) {
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
            *filename = column_text(rows->s, 0);
            assert(*filename != NULL);
            *timestamp = column_time_t(rows->s, 1);
            return 0;

        default:
            rowset_discard(rows);
            return -1;
    }
}

rowset_t *db_select_outputs(db_t *db, int id) {
    rowset_t *r = malloc(sizeof(*r));
    if (r == NULL)
        return NULL;

    char *getoutputs = "select filename, timestamp, mode, uid, gid, contents "
        "from output where fk_operation = @fk_operation;";
    if (prepare(db, &r->s, getoutputs) != SQLITE_OK) {
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
            *filename = column_text(rows->s, 0);
            assert(*filename != NULL);
            *timestamp = column_time_t(rows->s, 1);
            *mode = column_mode_t(rows->s, 2);
            *uid = column_uid_t(rows->s, 3);
            *gid = column_gid_t(rows->s, 4);
            *contents = column_text(rows->s, 5);
            return 0;

        default:
            rowset_discard(rows);
            return -1;
    }
}
