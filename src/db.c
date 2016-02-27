#include <assert.h>
#include "db.h"
#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
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

/* Helper to avoid having to remember to finalise statements all the time. */
static void autofinalize_(void *p) {
    sqlite3_stmt **s = p;
    if (*s != NULL)
        sqlite3_finalize(*s);
}
#define auto_sqlite3_stmt __attribute__((cleanup(autofinalize_))) sqlite3_stmt

int db_open(db_t *db, const char *path) {
    int r = sqlite3_open(path, &db->handle);
    if (r != SQLITE_OK)
        return -1;

    char *query =
        "create table if not exists trace ("
        "    id integer primary key autoincrement,"
        "    cwd text not null,"
        "    arg_lens blob not null,"
        "    arg_lens_sz integer not null,"
        "    argv text not null);"

        "create table if not exists input ("
        "    fk_trace integer references trace(id),"
        "    filename text not null,"
        "    timestamp integer not null);"

        "create table if not exists output ("
        "    fk_trace integer references trace(id),"
        "    filename text not null,"
        "    timestamp integer not null,"
        "    mode integer not null,"
        "    contents text not null);"
        
        "create table if not exists env ("
        "    fk_trace integer references trace(id),"
        "    name text not null,"
        "    value text);"
        
        "create table if not exists statistics ("
        "    fk_trace integer references trace(id),"
        "    event integer not null,"
        "    timestamp integer not null default current_timestamp);";
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
        "delete from trace;"
        "delete from env;"
        "delete from statistics;");
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

static int bind_blob(sqlite3_stmt *s, const char *param, const void *value,
        unsigned int size) {
    if (size > INT_MAX)
        /* Cast below will cause overflow. */
        return !SQLITE_OK;

    int index = sqlite3_bind_parameter_index(s, param);
    if (index == 0)
        return !SQLITE_OK;

    return sqlite3_bind_blob(s, index, value, (int)size, SQLITE_STATIC);
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

int db_select_id(db_t *db, int *id, const fingerprint_t *fp) {
    auto_sqlite3_stmt *s = NULL;
    char *getid = "select id from trace where cwd = @cwd and arg_lens = @arg_lens and arg_lens_sz = @arg_lens_sz and argv = @argv;";
    if (prepare(db, &s, getid) != SQLITE_OK)
        return -1;

    if (bind_text(s, "@cwd", fp->cwd) != SQLITE_OK ||
            bind_blob(s, "@arg_lens", fp->arg_lens, fp->arg_lens_sz * sizeof(*fp->arg_lens)) ||
            bind_int(s, "@arg_lens_sz", fp->arg_lens_sz) ||
            bind_text(s, "@argv", fp->argv) != SQLITE_OK)
        return -1;

    if (sqlite3_step(s) != SQLITE_ROW)
        return -1;

    assert(sqlite3_column_count(s) == 1);
    *id = column_int(s, 0);

    assert(sqlite3_step(s) == SQLITE_DONE);

    return 0;
}

int db_remove_id(db_t *db, int id) {
    {
        auto_sqlite3_stmt *s = NULL;
        char *deleteoutput = "delete from output where fk_trace = @id;";

        if (prepare(db, &s, deleteoutput) != SQLITE_OK)
            return -1;
        if (bind_int(s, "@id", id) != SQLITE_OK)
            return -1;
        if (sqlite3_step(s) != SQLITE_DONE)
            return -1;
    }

    {
        auto_sqlite3_stmt *s = NULL;
        char *deleteinput = "delete from input where fk_trace = @id;";

        if (prepare(db, &s, deleteinput) != SQLITE_OK)
            return -1;
        if (bind_int(s, "@id", id) != SQLITE_OK)
            return -1;
        if (sqlite3_step(s) != SQLITE_DONE)
            return -1;
    }

    {
        auto_sqlite3_stmt *s = NULL;
        char *deleteenv = "delete from env where fk_trace = @id;";

        if (prepare(db, &s, deleteenv) != SQLITE_OK)
            return -1;
        if (bind_int(s, "@id", id) != SQLITE_OK)
            return -1;
        if (sqlite3_step(s) != SQLITE_DONE)
            return -1;
    }

    {
        auto_sqlite3_stmt *s = NULL;
        char *deletestatistics = "delete from statistics where fk_trace = @id;";

        if (prepare(db, &s, deletestatistics) != SQLITE_OK)
            return -1;
        if (bind_int(s, "@id", id) != SQLITE_OK)
            return -1;
        if (sqlite3_step(s) != SQLITE_DONE)
            return -1;
    }

    {
        auto_sqlite3_stmt *s = NULL;
        char *deletetrace = "delete from trace where id = @id;";

        if (prepare(db, &s, deletetrace) != SQLITE_OK)
            return -1;
        if (bind_int(s, "@id", id) != SQLITE_OK)
            return -1;
        if (sqlite3_step(s) != SQLITE_DONE)
            return -1;
    }

    return 0;
}

int db_insert_event(db_t *db, int id, db_event_t event) {
    auto_sqlite3_stmt *s = NULL;
    char *add = "insert into statistics (fk_trace, event) values (@fk_trace, "
        "@event);";
    if (prepare(db, &s, add) != SQLITE_OK)
        return -1;

    if (bind_int(s, "@fk_trace", id) != SQLITE_OK ||
            bind_int(s, "@event", event) != SQLITE_OK)
        return -1;

    if (sqlite3_step(s) != SQLITE_DONE)
        return -1;

    return 0;
}

int db_insert_id(db_t *db, int *id, const fingerprint_t *fp) {
    auto_sqlite3_stmt *s = NULL;
    char *add = "insert into trace (cwd, arg_lens, arg_lens_sz, argv) values (@cwd, @arg_lens, @arg_lens_sz, @argv);";
    if (prepare(db, &s, add) != SQLITE_OK)
        return -1;

    if (bind_text(s, "@cwd", fp->cwd) != SQLITE_OK ||
            bind_blob(s, "@arg_lens", fp->arg_lens, fp->arg_lens_sz * sizeof(*fp->arg_lens)) ||
            bind_int(s, "@arg_lens_sz", fp->arg_lens_sz) ||
            bind_text(s, "@argv", fp->argv) != SQLITE_OK)
        return -1;

    if (sqlite3_step(s) != SQLITE_DONE)
        return -1;

    int result = db_select_id(db, id, fp);
    assert(result == 0);

    return result;
}

int db_insert_input(db_t *db, int id, const char *filename, time_t timestamp) {
    auto_sqlite3_stmt *s = NULL;
    char *add = "insert into input (fk_trace, filename, timestamp) values "
        "(@fk_trace, @filename, @timestamp);";
    if (prepare(db, &s, add) != SQLITE_OK)
        return -1;

    if (bind_int(s, "@fk_trace", id) != SQLITE_OK ||
            bind_text(s, "@filename", filename) != SQLITE_OK ||
            bind_time_t(s, "@timestamp", timestamp) != SQLITE_OK)
        return -1;

    if (sqlite3_step(s) != SQLITE_DONE)
        return -1;

    return 0;
}

int db_insert_output(db_t *db, int id, const char *filename, time_t timestamp,
        mode_t mode, const char *contents) {
    auto_sqlite3_stmt *s = NULL;
    char *add = "insert into output (fk_trace, filename, timestamp, mode, "
        "contents) values (@fk_trace, @filename, @timestamp, "
        "@mode, @contents);";
    if (prepare(db, &s, add) != SQLITE_OK)
        return -1;

    if (bind_int(s, "@fk_trace", id) != SQLITE_OK ||
            bind_text(s, "@filename", filename) != SQLITE_OK ||
            bind_time_t(s, "@timestamp", timestamp) != SQLITE_OK ||
            bind_mode_t(s, "@mode", mode) != SQLITE_OK ||
            bind_text(s, "@contents", contents) != SQLITE_OK)
        return -1;

    if (sqlite3_step(s) != SQLITE_DONE)
        return -1;

    return 0;
}

int db_insert_env(db_t *db, int id, const char *name, const char *value) {
    auto_sqlite3_stmt *s = NULL;
    char *add = "insert into env (fk_trace, name, value) values (@fk_trace, "
        "@name, @value);";
    if (prepare(db, &s, add) != SQLITE_OK)
        return -1;

    if (bind_int(s, "@fk_trace", id) != SQLITE_OK ||
            bind_text(s, "@name", name) != SQLITE_OK ||
            bind_text(s, "@value", value) != SQLITE_OK)
        return -1;

    if (sqlite3_step(s) != SQLITE_DONE)
        return -1;

    return 0;
}

int db_for_inputs(db_t *db, int id,
        int (*cb)(const char *filename, time_t timestamp)) {
    auto_sqlite3_stmt *s = NULL;

    char *getinputs = "select filename, timestamp from input where "
        "fk_trace = @fk_trace;";
    if (prepare(db, &s, getinputs) != SQLITE_OK)
        return -1;

    if (bind_int(s, "@fk_trace", id) != SQLITE_OK)
        return -1;

    while (true) {
        switch (sqlite3_step(s)) {
            case SQLITE_DONE:
                return 0;

            case SQLITE_ROW:
                assert(sqlite3_column_count(s) == 2);
                const char *filename = column_text(s, 0);
                assert(filename != NULL);
                time_t timestamp = column_time_t(s, 1);
                int r = cb(filename, timestamp);
                if (r != 0)
                    return r;
                break;

            default:
                return -1;
        }
    }

    assert(!"unreachable");
}

int db_for_outputs(db_t *db, int id,
        int (*cb)(const char *filename, time_t timestamp, mode_t mode,
        const char *contents)) {
    auto_sqlite3_stmt *s = NULL;

    char *getoutputs = "select filename, timestamp, mode, contents "
        "from output where fk_trace = @fk_trace;";
    if (prepare(db, &s, getoutputs) != SQLITE_OK)
        return -1;

    if (bind_int(s, "@fk_trace", id) != SQLITE_OK)
        return -1;

    while (true) {
        switch (sqlite3_step(s)) {
            case SQLITE_DONE:
                return 0;

            case SQLITE_ROW:
                assert(sqlite3_column_count(s) == 4);
                const char *filename = column_text(s, 0);
                assert(filename != NULL);
                time_t timestamp = column_time_t(s, 1);
                mode_t mode = column_mode_t(s, 2);
                const char *contents = column_text(s, 3);
                int r = cb(filename, timestamp, mode, contents);
                if (r != 0)
                    return r;
                break;

            default:
                return -1;
        }
    }

    assert(!"unreachable");
}

int db_for_env(db_t *db, int id,
        int (*cb)(const char *name, const char *value)) {
    auto_sqlite3_stmt *s = NULL;

    char *getenv = "select name, value from env where fk_trace = @fk_trace;";
    if (prepare(db, &s, getenv) != SQLITE_OK)
        return -1;

    if (bind_int(s, "@fk_trace", id) != SQLITE_OK)
        return -1;

    while (true) {
        switch (sqlite3_step(s)) {
            case SQLITE_DONE:
                return 0;

            case SQLITE_ROW:
                assert(sqlite3_column_count(s) == 2);
                const char *name = column_text(s, 0);
                assert(name != NULL);
                const char *value = column_text(s, 1);
                int r = cb(name, value);
                if (r != 0)
                    return r;
                break;

            default:
                return -1;
        }
    }

    assert(!"unreachable");
}
