#include <assert.h>
#include "cache.h"
#include "depset.h"
#include "dict.h"
#include <fcntl.h>
#include "filehash.h"
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

/* SQLite documentation advises to use sqlite3_prepare_v2 in preference to
 * sqlite3_prepare.
 */
#define sqlite3_prepare sqlite3_prepare_v2

/* Subdirectory of the cache root under which to cache file contents of output
 * files.
 */
#define DATA "/data"

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
    int index = sqlite3_bind_parameter_index(s, "cwd");
    if (index == 0)
        goto fail;
    if (sqlite3_bind_text(s, index, cwd, -1, SQLITE_STATIC) != SQLITE_OK)
        goto fail;
    index = sqlite3_bind_parameter_index(s, "command");
    if (index == 0)
        goto fail;
    if (sqlite3_bind_text(s, index, command, -1, SQLITE_STATIC) != SQLITE_OK)
        goto fail;

    /* OK go. */
    if (sqlite3_step(s) != SQLITE_ROW)
        /* This cwd/command combination was not in the cache. */
        goto fail;
    assert(sqlite3_column_count(s) == 1);
    assert(sqlite3_column_type(s, 1) == SQLITE_INTEGER);
    int id = sqlite3_column_int(s, 1);

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
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        return NULL;

    /* Measure the size of the file we're about to cache. */
    struct stat st;
    int r = fstat(fd, &st);
    if (r != 0) {
        close(fd);
        return NULL;
    }
    size_t sz = st.st_size;

    /* Mmap the file purely for the purposes of calculating its hash. */
    void *addr = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return NULL;
    }
    char *h = filehash((char*)addr, sz);
    munmap(addr, sz);
    if (h == NULL) {
        close(fd);
        return NULL;
    }

    /* Copy the file itself to the cache. Note that we do this through the
     * kernel (sendfile) for efficiency, but we've actually already read the
     * entire file in userspace when we just calculated its hash. It may be
     * more efficient to do the copy in userspace at this point.
     */
    char *cpath = (char*)malloc(strlen(c->root) + strlen(DATA) + 1 + strlen(h)
        + 1);
    if (cpath == NULL) {
        close(fd);
        return NULL;
    }
    sprintf(cpath, "%s" DATA "/%s", c->root, h);
    int out = open(cpath, O_WRONLY|O_CREAT);
    if (out < 0) {
        free(cpath);
        close(fd);
        return NULL;
    }
    ssize_t written = sendfile(out, fd, 0, sz);
    close(out);
    free(cpath);
    close(fd);
    if ((size_t)written != sz)
        /* We somehow failed to copy the entire file. FIXME: We're potentially
         * leaving (benign) garbage in the cache here that we should be
         * removing.
         */
        return NULL;
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
        int index = sqlite3_bind_parameter_index(s, "cwd");
        if (index == 0)
            goto fail;
        if (sqlite3_bind_text(s, index, cwd, -1, SQLITE_STATIC) != SQLITE_OK)
            goto fail;
        index = sqlite3_bind_parameter_index(s, "command");
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
    int id_index = sqlite3_bind_parameter_index(s, "fk_operation");
    if (id_index == 0)
        goto fail;
    int key_index = sqlite3_bind_parameter_index(s, "filename");
    if (key_index == 0)
        goto fail;
    int value_index = sqlite3_bind_parameter_index(s, "timestamp");
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
    id_index = sqlite3_bind_parameter_index(s, "fk_operation");
    if (id_index == 0)
        goto fail;
    key_index = sqlite3_bind_parameter_index(s, "filename");
    if (key_index == 0)
        goto fail;
    value_index = sqlite3_bind_parameter_index(s, "timestamp");
    if (value_index == 0)
        goto fail;
    int contents_index = sqlite3_bind_parameter_index(s, "contents");
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
    char *command = (char*)malloc(sz);
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
    free(command);
    free(cwd);
    if (id == -1)
        return -1;
    sqlite3_stmt *s = NULL;
    if (sqlite3_prepare(cache->db, query_getinputs, -1, &s, NULL) != SQLITE_OK)
        goto fail;
    /* TODO: This, and all the other, bind_parameter_indexes can be done at
     * compile-time.
     */
    int index = sqlite3_bind_parameter_index(s, "fk_operation");
    if (index == 0)
        goto fail;
    if (sqlite3_bind_int(s, index, id) != SQLITE_OK)
        goto fail;
    int r;
    while ((r = sqlite3_step(s)) == SQLITE_ROW) {
        assert(sqlite3_column_count(s) == 2);
        assert(sqlite3_column_type(s, 1) == SQLITE_TEXT);
        const char *filename = (const char*)sqlite3_column_text(s, 1);
        assert(filename != NULL);
        assert(sqlite3_column_type(s, 2) == SQLITE_INTEGER);
        time_t timestamp = (time_t)sqlite3_column_int64(s, 2);
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

    /* We found it with matching inputs. */
    return id;

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
