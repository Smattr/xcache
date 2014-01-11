#include <assert.h>
#include "db.h"
#include "macros.h"
#include "queries.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Expand the name of a query file in sql/ into the symbol name xxd generates
 * for that query.
 */
#define QUERY(query) __##query##_sql

struct db {
    sqlite3 *handle;
};

/* Wrapper around sqlite3_prepare_v2. */
static int _prepare(db_t *db, sqlite3_stmt **s, const char *query, int len) {
    return sqlite3_prepare_v2(db->handle, query, len, s, NULL);
}
#define prepare(db, s, query) _prepare((db), (s), (const char*)(query), JOIN(query, _len))

/* Wrapper around sqlite3_exec. */
static int _exec(db_t *db, const char *query, int len) {
    char *q;
    if (len == -1) {
        q = strdup(query);
        if (q == NULL)
            return -1;
    } else {
        q = malloc(len + 1);
        if (q == NULL)
            return -1;
        strncpy(q, query, len);
        q[len] = '\0';
    }

    int r = sqlite3_exec(db->handle, q, NULL, NULL, NULL);
    free(q);
    return r;
}
#define exec(db, query) _exec((db), (const char*)(query), JOIN(query, _len))

db_t *db_open(const char *path) {
    db_t *d = (db_t*)malloc(sizeof(*d));
    if (d == NULL)
        return NULL;

    int r = sqlite3_open(path, &d->handle);
    if (r != SQLITE_OK) {
        free(d);
        return NULL;
    }

    if (exec(d, QUERY(create)) != 0) {
        db_close(d);
        free(d);
        return NULL;
    }

    return d;
}

int db_begin(db_t *db) {
    return _exec(db, "begin transaction", -1);
}
int db_commit(db_t *db) {
    return _exec(db, "commit transaction", -1);
}
int db_rollback(db_t *db) {
    return _exec(db, "rollback transaction", -1);
}

int db_clear(db_t *db) {
    return exec(db, QUERY(truncate));
}

int db_close(db_t *db) {
    if (sqlite3_close(db->handle) != SQLITE_OK)
        return -1;
    free(db);
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
    if (prepare(db, &s, QUERY(getid)) != SQLITE_OK)
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

    if (prepare(db, &s, QUERY(deleteoutput)) != SQLITE_OK)
        goto fail;
    if (bind_int(s, "@fk_operation", id) != SQLITE_OK)
        goto fail;
    if (sqlite3_step(s) != SQLITE_DONE)
        goto fail;
    sqlite3_finalize(s);
    s = NULL;

    if (prepare(db, &s, QUERY(deleteinput)) != SQLITE_OK)
        goto fail;
    if (bind_int(s, "@fk_operation", id) != SQLITE_OK)
        goto fail;
    if (sqlite3_step(s) != SQLITE_DONE)
        goto fail;
    sqlite3_finalize(s);
    s = NULL;

    if (prepare(db, &s, QUERY(deleteoperation)) != SQLITE_OK)
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
    if (prepare(db, &s, QUERY(addoperation)) != SQLITE_OK)
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
    if (prepare(db, &s, QUERY(addinput)) != SQLITE_OK)
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
    if (prepare(db, &s, QUERY(addoutput)) != SQLITE_OK)
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

    if (prepare(db, &r->s, QUERY(getinputs)) != SQLITE_OK) {
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
    rowset_t *r = (rowset_t*)malloc(sizeof(*r));
    if (r == NULL)
        return NULL;

    if (prepare(db, &r->s, QUERY(getoutputs)) != SQLITE_OK) {
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
