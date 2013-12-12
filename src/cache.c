#include <assert.h>
#include "cache.h"
#include "depset.h"
#include "dict.h"
#include <fcntl.h>
#include "file.h"
#include "log.h"
#include "queries.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

/* SQLite documentation advises to use sqlite3_prepare_v2 in preference to
 * sqlite3_prepare.
 */
#define sqlite3_prepare sqlite3_prepare_v2

/* Subdirectory of the cache root under which to cache file contents of output
 * files.
 */
#define DATA "/data"

#define DB   "cache.db"

struct cache {

    /* Underlying data store for metadata about dependency graphs. */
    sqlite3 *db;

    /* Absolute path (without a trailing slash) to the directory to store cache
     * metadata and file data in.
     */
    char *root;

};

cache_t *cache_open(const char *path) {
    cache_t *c = (cache_t*)malloc(sizeof(cache_t));
    if (c == NULL) {
        return NULL;
    }

    char *db_path = (char*)malloc(strlen(path) + 1 + strlen(DB) + 1);
    if (db_path == NULL) {
        free(c);
        return NULL;
    }
    sprintf(db_path, "%s/" DB, path);
    int r = sqlite3_open(db_path, &c->db);
    free(db_path);
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

/* Some wrappers around SQLite's verbose query execution. */
static bool begin_transaction(cache_t *c) {
    return sqlite3_exec(c->db, "begin transaction;", NULL, NULL, NULL)
        == SQLITE_OK;
}
static bool commit_transaction(cache_t *c) {
    return sqlite3_exec(c->db, "commit transaction;", NULL, NULL, NULL)
        == SQLITE_OK;
}
static bool rollback_transaction(cache_t *c) {
    return sqlite3_exec(c->db, "rollback transaction;", NULL, NULL, NULL)
        == SQLITE_OK;
}

static int get_id(cache_t *c, char *cwd, char *command) {
    sqlite3_stmt *s;

    /* Prepare select statement. */
    if (sqlite3_prepare(c->db, query_getid, -1, &s, NULL) != SQLITE_OK)
        return -1;

    /* Bind the parameters. */
    int index = sqlite3_bind_parameter_index(s, "@cwd");
    if (index == 0)
        goto fail;
    if (sqlite3_bind_text(s, index, cwd, -1, SQLITE_STATIC) != SQLITE_OK)
        goto fail;
    index = sqlite3_bind_parameter_index(s, "@command");
    if (index == 0)
        goto fail;
    if (sqlite3_bind_text(s, index, command, -1, SQLITE_STATIC) != SQLITE_OK)
        goto fail;

    /* OK go. */
    if (sqlite3_step(s) != SQLITE_ROW)
        /* This cwd/command combination was not in the cache. */
        goto fail;
    assert(sqlite3_column_count(s) == 1);
    assert(sqlite3_column_type(s, 0) == SQLITE_INTEGER);
    int id = sqlite3_column_int(s, 0);

    /* There should have only been a single row because cwd/command is intended
     * to be unique.
     */
    assert(sqlite3_step(s) == SQLITE_DONE);

    sqlite3_finalize(s);
    return id;

fail:
    sqlite3_finalize(s);
    return -1;
}

/* Save a file to the cache.
 *
 * c - The cache to save to.
 * filename - Absolute path to the file to copy into the cache.
 *
 * Returns NULL on failure or the hash of the file data on success. It is the
 * caller's responsibility to free the returned pointer.
 *
 * TODO: Place some limit on the size of file that can be saved.
 */
static char *cache_save(cache_t *c, char *filename) {
    char *h = filehash(filename);
    if (h == NULL)
        return NULL;

    char *cpath = (char*)malloc(strlen(c->root) + strlen(DATA) + 1 + strlen(h)
        + 1);
    if (cpath == NULL) {
        free(h);
        return NULL;
    }
    sprintf(cpath, "%s" DATA "/%s", c->root, h);
    if (cp(filename, cpath) != 0) {
        free(cpath);
        free(h);
        return NULL;
    }
    free(cpath);
    return h;
}

int cache_write(cache_t *cache, char *cwd, char *command, depset_t *depset) {
    sqlite3_stmt *s = NULL;
    iter_t *i = NULL;
    if (!begin_transaction(cache)) {
        return -1;
    }

    int id = get_id(cache, cwd, command);
    if (id < 0) {
        /* This entry was not found in the cache. */
        /* Write the entry to the dep table. */
        if (sqlite3_prepare(cache->db, query_addoperation, -1, &s, NULL) != SQLITE_OK)
            goto fail;
        int index = sqlite3_bind_parameter_index(s, "@cwd");
        if (index == 0)
            goto fail;
        if (sqlite3_bind_text(s, index, cwd, -1, SQLITE_STATIC) != SQLITE_OK)
            goto fail;
        index = sqlite3_bind_parameter_index(s, "@command");
        if (index == 0)
            goto fail;
        if (sqlite3_bind_text(s, index, command, -1, SQLITE_STATIC) != SQLITE_OK)
            goto fail;
        if (sqlite3_step(s) != SQLITE_DONE)
            goto fail;
        if (sqlite3_finalize(s) != SQLITE_OK)
            goto fail;
        s = NULL;
        id = get_id(cache, cwd, command);
    }

    assert(id >= 0);

    /* Write the inputs. */
    i = depset_iter_inputs(depset);
    if (i == NULL)
        goto fail;
    char *key;
    time_t *value;
    if (sqlite3_prepare(cache->db, query_addinput, -1, &s, NULL) != SQLITE_OK)
        goto fail;
    int id_index = sqlite3_bind_parameter_index(s, "@fk_operation");
    if (id_index == 0)
        goto fail;
    int key_index = sqlite3_bind_parameter_index(s, "@filename");
    if (key_index == 0)
        goto fail;
    int value_index = sqlite3_bind_parameter_index(s, "@timestamp");
    if (value_index == 0)
        goto fail;
    while (iter_next(i, &key, (void**)&value)) {
        if (sqlite3_bind_int(s, id_index, id) != SQLITE_OK)
            goto fail;
        if (sqlite3_bind_text(s, key_index, key, -1, SQLITE_STATIC) != SQLITE_OK)
            goto fail;
        if (sqlite3_bind_int64(s, value_index, (sqlite3_int64)(value)) != SQLITE_OK)
            goto fail;
        if (sqlite3_step(s) != SQLITE_DONE)
            goto fail;
        if (sqlite3_reset(s) != SQLITE_OK)
            goto fail;
    }
    i = NULL;
    if (sqlite3_finalize(s) != SQLITE_OK)
        goto fail;
    s = NULL;

    /* Write the outputs. */
    i = depset_iter_outputs(depset);
    if (i == NULL)
        goto fail;
    if (sqlite3_prepare(cache->db, query_addoutput, -1, &s, NULL) != SQLITE_OK)
        goto fail;
    id_index = sqlite3_bind_parameter_index(s, "@fk_operation");
    if (id_index == 0)
        goto fail;
    key_index = sqlite3_bind_parameter_index(s, "@filename");
    if (key_index == 0)
        goto fail;
    value_index = sqlite3_bind_parameter_index(s, "@timestamp");
    if (value_index == 0)
        goto fail;
    int contents_index = sqlite3_bind_parameter_index(s, "@contents");
    if (contents_index == 0)
        goto fail;
    while (iter_next(i, &key, (void**)&value)) {
        char *h = cache_save(cache, key);
        if (h == NULL)
            goto fail;

        if (sqlite3_bind_int(s, id_index, id) != SQLITE_OK)
            goto fail;
        if (sqlite3_bind_text(s, key_index, key, -1, SQLITE_STATIC) != SQLITE_OK)
            goto fail;
        if (sqlite3_bind_int64(s, value_index, (sqlite3_int64)(value)) != SQLITE_OK)
            goto fail;
        if (sqlite3_bind_text(s, contents_index, h, -1, free) != SQLITE_OK)
            goto fail;
        if (sqlite3_step(s) != SQLITE_DONE)
            goto fail;
        if (sqlite3_reset(s) != SQLITE_OK)
            goto fail;
    }
    i = NULL;
    if (sqlite3_finalize(s) != SQLITE_OK)
        goto fail;
    s = NULL;

    if (!commit_transaction(cache))
        goto fail;
    return 0;

fail:
    if (s != NULL)
        sqlite3_finalize(s);
    if (i != NULL)
        iter_destroy(i);
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

int cache_locate(cache_t *cache, char **args) {
    size_t sz = strlen(args[0]) + 1;
    char *command = strdup(args[0]);
    if (command == NULL)
        return -1;
    for (unsigned int i = 1; args[i] != NULL; i++) {
        sz += strlen(args[i]) + 1;
        char *tmp = (char*)realloc(command, sz);
        if (tmp == NULL) {
            free(command);
            return -1;
        }
        strcat(command, " ");
        strcat(command, args[i]);
    }
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        free(command);
        return -1;
    }

    int id = get_id(cache, cwd, command);
    if (id == -1) {
        DEBUG("Failed to locate cache entry for \"%s\" in directory \"%s\"\n",
            command, cwd);
        free(command);
        free(cwd);
        return -1;
    }
    free(command);
    free(cwd);
    sqlite3_stmt *s = NULL;
    if (sqlite3_prepare(cache->db, query_getinputs, -1, &s, NULL) != SQLITE_OK)
        goto fail;
    /* TODO: This, and all the other, bind_parameter_indexes can be done at
     * compile-time.
     */
    int index = sqlite3_bind_parameter_index(s, "@fk_operation");
    if (index == 0)
        goto fail;
    if (sqlite3_bind_int(s, index, id) != SQLITE_OK)
        goto fail;
    int r;
    while ((r = sqlite3_step(s)) == SQLITE_ROW) {
        assert(sqlite3_column_count(s) == 2);
        assert(sqlite3_column_type(s, 0) == SQLITE_TEXT);
        const char *filename = (const char*)sqlite3_column_text(s, 0);
        assert(filename != NULL);
        assert(sqlite3_column_type(s, 1) == SQLITE_INTEGER);
        time_t timestamp = (time_t)sqlite3_column_int64(s, 1);
        struct stat st;
        if (stat(filename, &st) != 0)
            goto fail;
        if (st.st_mtime != timestamp)
            /* This is actually the expected case; that we found the input file
             * but its timestamp has changed.
             */
            goto fail;
    }
    if (r != SQLITE_DONE)
        goto fail;
    sqlite3_finalize(s);
    s = NULL;

    /* We found it with matching inputs. */
    return id;

fail:
    if (s != NULL)
        sqlite3_finalize(s);
    return -1;
}

int cache_dump(cache_t *cache, int id) {
    sqlite3_stmt *s = NULL;

    assert(cache != NULL);
    if (sqlite3_prepare(cache->db, query_getoutputs, -1, &s, NULL) != SQLITE_OK)
        goto fail;
    int index = sqlite3_bind_parameter_index(s, "@fk_operation");
    if (index == 0)
        goto fail;
    if (sqlite3_bind_int(s, index, id) != SQLITE_OK)
        goto fail;
    int r;
    while ((r = sqlite3_step(s)) == SQLITE_ROW) {
        assert(sqlite3_column_count(s) == 3);
        assert(sqlite3_column_type(s, 0) == SQLITE_TEXT);
        const char *filename = (const char*)sqlite3_column_text(s, 0);
        assert(filename != NULL);
        assert(sqlite3_column_type(s, 1) == SQLITE_INTEGER);
        time_t timestamp = (time_t)sqlite3_column_int64(s, 1);
        assert(sqlite3_column_type(s, 2) == SQLITE_TEXT);
        const char *contents = (const char*)sqlite3_column_text(s, 2);

        char *last_slash = strrchr(filename, '/');
        /* The path should contain at least one slash because it should be
         * absolute.
         */
        assert(last_slash != NULL);
        if (filename != last_slash) {
            /* We're not creating a file in the root directory. */
            last_slash[0] = '\0';
            int m = mkdirp(filename);
            if (m != 0)
                goto fail;
            last_slash[0] = '/';
        }

        int out = open(filename, O_CREAT|O_WRONLY);
        if (out < 0)
            goto fail;
        char *cached_copy = (char*)malloc(strlen(cache->root) + 1 + strlen(contents) + 1);
        if (cached_copy == NULL) {
            close(out);
            goto fail;
        }
        sprintf(cached_copy, "%s/%s", cache->root, contents);
        int in = open(cached_copy, O_RDONLY);
        if (in < 0) {
            free(cached_copy);
            close(out);
            goto fail;
        }
        struct stat st;
        if (fstat(in, &st) != 0) {
            close(in);
            free(cached_copy);
            close(out);
            goto fail;
        }
        ssize_t copied = sendfile(out, in, NULL, st.st_size);
        close(in);
        free(cached_copy);
        fchown(out, st.st_uid, st.st_gid);
        close(out);
        struct utimbuf ut = {
            .actime = timestamp,
            .modtime = timestamp,
        };
        utime(filename, &ut);
        if (copied != st.st_size)
            goto fail;
    }
    if (r != SQLITE_DONE)
        goto fail;
    sqlite3_finalize(s);
    s = NULL;

    return 0;

fail:
    if (s != NULL)
        sqlite3_finalize(s);
    return -1;
}

int cache_close(cache_t *cache) {
    assert(cache != NULL);
    int r = sqlite3_close(cache->db);
    if (r != SQLITE_OK) {
        return -1;
    }
    return 0;
}
