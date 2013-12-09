#include <assert.h>
#include "cache.h"
#include "depset.h"
#include "dict.h"
#include "queries.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define sqlite3_prepare sqlite3_prepare_v2

#define DATA "/data"

#if __WORDSIZE == 32
    #define sqlite3_bind_timestamp(stmt, index, val) \
        sqlite3_bind_int(stmt, index, val)
#else
    #define sqlite3_bind_timestamp(stmt, index, val) \
        sqlite3_bind_int64(stmt, index, (sqlite3_int64)(val))
#endif

struct cache {
    sqlite3 *db;
    char *root;
};

cache_t *cache_open(const char *path) {
    cache_t *c = (cache_t*)malloc(sizeof(cache_t));
    if (c == NULL) {
        return NULL;
    }

    int r = sqlite3_open(path, &c->db);
    if (r != SQLITE_OK) {
        sqlite3_close(c->db);
        free(c);
        return NULL;
    }

    r = sqlite3_exec(c->db, query_create, NULL, NULL, NULL);
    if (r != SQLITE_OK) {
        sqlite3_close(c->db);
        free(c);
        return NULL;
    }

    c->root = (char*)malloc(strlen(path) + strlen(DATA) + 1);
    if (c->root == NULL) {
        sqlite3_close(c->db);
        free(c);
        return NULL;
    }
    sprintf(c->root, "%s" DATA, path);

    return c;
}

static bool begin_transaction(cache_t *c) {
    return sqlite3_exec(c->db, "begin transaction;", NULL, NULL, NULL) == SQLITE_OK;
}
static bool commit_transaction(cache_t *c) {
    return sqlite3_exec(c->db, "commit transaction;", NULL, NULL, NULL) == SQLITE_OK;
}
static bool rollback_transaction(cache_t *c) {
    return sqlite3_exec(c->db, "rollback transaction;", NULL, NULL, NULL) == SQLITE_OK;
}

int cache_write(cache_t *cache, char *cwd, char *command, depset_t *depset) {
    if (!begin_transaction(cache)) {
        return -1;
    }

    /* Write the entry to the dep table. */
    sqlite3_stmt *st;
    if (sqlite3_prepare(cache->db, query_addoperation, -1, &st, NULL) != SQLITE_OK)
        goto fail;
    int index = sqlite3_bind_parameter_index(st, "cwd");
    if (index == 0) {
        sqlite3_finalize(st);
        goto fail;
    }
    if (sqlite3_bind_text(st, index, cwd, -1, SQLITE_STATIC) != SQLITE_OK) {
        sqlite3_finalize(st);
        goto fail;
    }
    index = sqlite3_bind_parameter_index(st, "command");
    if (index == 0) {
        sqlite3_finalize(st);
        goto fail;
    }
    if (sqlite3_bind_text(st, index, command, -1, SQLITE_STATIC) != SQLITE_OK) {
        sqlite3_finalize(st);
        goto fail;
    }
    if (sqlite3_step(st) != SQLITE_DONE) {
        sqlite3_finalize(st);
        goto fail;
    }
    if (sqlite3_finalize(st) != SQLITE_OK)
        goto fail;

    /* Write the inputs. */
    iter_t *i = depset_iter_inputs(depset);
    if (i == NULL)
        goto fail;
    /* TODO: free i in following code */
    /* TODO: bind fk_operation */
    char *key;
    time_t *value;
    if (sqlite3_prepare(cache->db, query_addinput, -1, &st, NULL) != SQLITE_OK)
        goto fail;
    int key_index = sqlite3_bind_parameter_index(st, "filename");
    if (key_index == 0) {
        sqlite3_finalize(st);
        goto fail;
    }
    int value_index = sqlite3_bind_parameter_index(st, "timestamp");
    if (value_index == 0) {
        sqlite3_finalize(st);
        goto fail;
    }
    while (iter_next(i, &key, (void**)&value)) {
        if (sqlite3_bind_text(st, key_index, key, -1, SQLITE_STATIC) != SQLITE_OK) {
            sqlite3_finalize(st);
            goto fail;
        }
        if (sqlite3_bind_timestamp(st, value_index, value) != SQLITE_OK) {
            sqlite3_finalize(st);
            goto fail;
        }
        if (sqlite3_step(st) != SQLITE_DONE) {
            sqlite3_finalize(st);
            goto fail;
        }
        if (sqlite3_reset(st) != SQLITE_OK) {
            sqlite3_finalize(st);
            goto fail;
        }
    }
    if (sqlite3_finalize(st) != SQLITE_OK)
        goto fail;

    /* Write the outputs. */
    i = depset_iter_outputs(depset);
    if (i == NULL)
        goto fail;
    if (sqlite3_prepare(cache->db, query_addoutput, -1, &st, NULL) != SQLITE_OK)
        goto fail;
    key_index = sqlite3_bind_parameter_index(st, "filename");
    if (key_index == 0) {
        sqlite3_finalize(st);
        goto fail;
    }
    value_index = sqlite3_bind_parameter_index(st, "timestamp");
    if (value_index == 0) {
        sqlite3_finalize(st);
        goto fail;
    }
    while (iter_next(i, &key, (void**)&value)) {
        if (sqlite3_bind_text(st, key_index, key, -1, SQLITE_STATIC) != SQLITE_OK) {
            sqlite3_finalize(st);
            goto fail;
        }
        if (sqlite3_bind_timestamp(st, value_index, value) != SQLITE_OK) {
            sqlite3_finalize(st);
            goto fail;
        }
        if (sqlite3_step(st) != SQLITE_DONE) {
            sqlite3_finalize(st);
            goto fail;
        }
        if (sqlite3_reset(st) != SQLITE_OK) {
            sqlite3_finalize(st);
            goto fail;
        }
        /* TODO write file data */
    }
    if (sqlite3_finalize(st) != SQLITE_OK)
        goto fail;

    if (!commit_transaction(cache)) {
        return -1;
    }
    return 0;

fail:
    rollback_transaction(cache);
    return -1;
}

int cache_clear(cache_t *cache) {
    assert(cache != NULL);
    int r = sqlite3_exec(cache->db, query_truncate, NULL, NULL, NULL);
    if (r != SQLITE_OK) {
        return -1;
    }
    return 0;
}

int cache_close(cache_t *cache) {
    assert(cache != NULL);
    int r = sqlite3_close(cache->db);
    if (r != SQLITE_OK) {
        return -1;
    }
    return 0;
}
