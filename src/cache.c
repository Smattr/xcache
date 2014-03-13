#define _GNU_SOURCE /* for asprintf */
#include <assert.h>
#include "cache.h"
#include "collection/dict.h"
#include "constants.h"
#include "db.h"
#include "depset.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include "log.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"
#include <utime.h>

/* Subdirectory of the cache root under which to cache file contents of output
 * files.
 */
#define DATA "/data"

#define DB   "cache.db"

struct cache {

    /* Underlying data store for metadata about dependency graphs. */
    db_t *db;

    /* Absolute path (without a trailing slash) to the directory to store cache
     * metadata and file data in.
     */
    char *root;

    /* Size limit to place on the data section of the cache. Older entries will
     * be evicted to make space for newer entries. This size limit does *not*
     * include the space used by the SQLite database.
     */
    ssize_t limit; /* bytes */

    /* Current size of the data section of the cache. Only gets initialised
     * when we're not using an unlimited cache.
     */
    ssize_t size; /* bytes */

};

cache_t *cache_open(const char *path, ssize_t limit) {
    cache_t *c = (cache_t*)malloc(sizeof(cache_t));
    if (c == NULL)
        return NULL;

    char *db_path;
    int err = asprintf(&db_path, "%s/" DB, path);
    if (err == -1) {
        free(c);
        return NULL;
    }
    c->db = db_open(db_path);
    free(db_path);
    if (c->db == NULL) {
        free(c);
        return NULL;
    }

    err = asprintf(&c->root, "%s" DATA, path);
    if (err == -1) {
        db_close(c->db);
        free(c);
        return NULL;
    }
    if (mkdirp(c->root) != 0) {
        free(c->root);
        db_close(c->db);
        free(c);
        return NULL;
    }

    if (limit == DATA_SIZE_UNSET)
        c->limit = DATA_LIMIT_DEFAULT;
    else {
        c->limit = limit;
        /* We'll need to stat every file in the cache to measure its current
         * size if we ever save anything, but we may never need to, so let's
         * just do this on demand later.
         */
        c->size = DATA_SIZE_UNSET;
    }

    return c;
}

static int get_id(cache_t *c, char *cwd, char *command) {
    int id;
    if (db_select_id(c->db, &id, cwd, command) != 0)
        return -1;
    return id;
}

/* Save a file to the cache.
 *
 * c - The cache to save to.
 * filename - Absolute path to the file to copy into the cache.
 *
 * Returns NULL on failure or the hash of the file data on success. It is the
 * caller's responsibility to free the returned pointer.
 */
static char *cache_save(cache_t *c, const char *filename) {
    char *h = filehash(filename);
    if (h == NULL)
        return NULL;

    if (c->size != DATA_LIMIT_UNLIMITED) {
        /* Only take the overhead of an extra stat here to check the filesize
         * if the cache is not unlimited.
         */
        struct stat st;
        if (stat(filename, &st) != 0)
            return NULL;
        if (st.st_size > c->limit)
            /* This file would never fit in the cache, even if it were empty.
             */
            return NULL;
        if (c->size == DATA_SIZE_UNSET) {
            /* We haven't measured the cache yet. Time for a coffee. */
            c->size = du(c->root);
            if (c->size == -1) {
                /* Failed to measure the cache. Who knows why. */
                c->size = DATA_SIZE_UNSET;
                return NULL;
            }
        }
        /* We must have measured the cache by here. */
        assert(c->size != DATA_SIZE_UNSET);
        if (st.st_size + c->size > c->limit) {
            /* This file will not fit in the cache as it stands. */
            ssize_t removed = reduce(c->root, st.st_size + c->size - c->limit);
            if (removed == -1)
                /* We failed to prune the cache. */
                return NULL;
            c->size -= removed;
            /* We should now be able to fit this item. */
            assert(c->size + st.st_size <= c->limit);
        }
    }

    char *cpath;
    int err = asprintf(&cpath, "%s/%s", c->root, h);
    if (err == -1) {
        free(h);
        return NULL;
    }
    if (cp(filename, cpath) != 0) {
        free(cpath);
        free(h);
        return NULL;
    }
    free(cpath);
    return h;
}

static char *to_command(const char **args) {
    assert(args != NULL);
    assert(args[0] != NULL);

    size_t sz = strlen(args[0]) + 1;
    char *command = strdup(args[0]);
    if (command == NULL)
        return NULL;
    for (unsigned int i = 1; args[i] != NULL; i++) {
        sz += strlen(args[i]) + 1;
        char *tmp = (char*)realloc(command, sz);
        if (tmp == NULL) {
            free(command);
            return NULL;
        }
        command = tmp;
        strcat(command, " ");
        strcat(command, args[i]);
    }
    return command;
}

int cache_write(cache_t *cache, char *cwd, const char **args,
        depset_t *depset, const char *outfile, const char *errfile) {
    char *command = to_command(args);
    if (command == NULL)
        return -1;
    dict_iter_t *di = NULL;
    set_iter_t *si = NULL;
    if (db_begin(cache->db) != 0) {
        free(command);
        return -1;
    }

    int id = get_id(cache, cwd, command);
    if (id < 0) {
        /* This entry was not found in the cache. */
        /* Write the entry to the dep table. */
        if (db_insert_id(cache->db, &id, cwd, command) != 0)
            goto fail;
    }
    free(command);
    command = NULL;

    assert(id >= 0);

    /* Write the inputs. */
    di = depset_iter_inputs(depset);
    if (di == NULL)
        goto fail;
    char *key;
    time_t value;
    while (dict_iter_next(di, &key, (void**)&value) == 0) {
        if (db_insert_input(cache->db, id, key, value) != 0)
            goto fail;
    }
    di = NULL;

    /* Write the outputs. */
    si = depset_iter_outputs(depset);
    if (si == NULL)
        goto fail;
    while (set_iter_next(si, (const char**)&key) == 0) {
        struct stat st;
        if (stat(key, &st) != 0)
            continue;

        char *h = cache_save(cache, key);
        if (h == NULL)
            goto fail;

        int r = db_insert_output(cache->db, id, key, st.st_mtime, st.st_mode,
            st.st_uid, st.st_gid, h);
        free(h);
        if (r != 0)
            goto fail;
    }
    si = NULL;

    if (outfile != NULL) {
        char *h = cache_save(cache, outfile);
        if (h == NULL)
            goto fail;

        int r = db_insert_output(cache->db, id, "/dev/stdout", 0, 0, 0, 0, h);
        free(h);
        if (r != 0)
            goto fail;
    }

    if (errfile != NULL) {
        char *h = cache_save(cache, errfile);
        if (h == NULL)
            goto fail;

        int r = db_insert_output(cache->db, id, "/dev/stderr", 0, 0, 0, 0, h);
        free(h);
        if (r != 0)
            goto fail;
    }

    if (db_commit(cache->db) != 0)
        goto fail;
    return 0;

fail:
    if (di != NULL)
        dict_iter_destroy(di);
    if (si != NULL)
        set_iter_destroy(si);
    db_rollback(cache->db);
    if (command != NULL)
        free(command);
    return -1;
}

int cache_clear(cache_t *cache) {
    return db_clear(cache->db);
}

int cache_locate(cache_t *cache, const char **args) {
    char *command = to_command(args);
    if (command == NULL)
        return -1;
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

    rowset_t *r = db_select_inputs(cache->db, id);
    if (r == NULL)
        goto fail;
    const char *filename;
    time_t timestamp;
    while (rowset_next_input(r, &filename, &timestamp) == 0) {
        struct stat st;
        if (stat(filename, &st) != 0) {
            if (errno == ENOENT && timestamp == MISSING)
                /* The file doesn't exist, but we expected it not to. */
                continue;
            DEBUG("Failed to stat %s\n", filename);
            goto fail;
        } else if (st.st_mtime != timestamp) {
            /* This is actually the expected case; that we found the input file
             * but its timestamp has changed.
             */
            DEBUG("Found %s but its timestamp was %llu, not %llu as expected\n",
                filename, (long long unsigned)st.st_mtime,
                (long long unsigned)timestamp);
            goto fail;
        }
    }
    r = NULL;

    /* We found it with matching inputs. */
    return id;

fail:
    if (r != NULL)
        rowset_discard(r);
    return -1;
}

int cache_dump(cache_t *cache, int id) {
    rowset_t *r = db_select_outputs(cache->db, id);
    if (r == NULL)
        return -1;

    const char *filename, *contents;
    time_t timestamp;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    while (rowset_next_output(r, &filename, &timestamp, &mode, &uid, &gid,
            &contents) == 0) {
        char *last_slash = strrchr(filename, '/');
        /* The path should contain at least one slash because it should be
         * absolute.
         */
        assert(last_slash != NULL);
        if (filename != last_slash) {
            /* We're not creating a file in the root directory. */
            last_slash[0] = '\0';
            int m = mkdirp(filename);
            if (m != 0) {
                ERROR("Failed to create directory %s\n", filename);
                goto fail;
            }
            last_slash[0] = '/';
        }

        char *cached_copy;
        int err = asprintf(&cached_copy, "%s/%s", cache->root, contents);
        if (err == -1) {
            ERROR("Out of memory while dumping cache entry %s\n", filename);
            goto fail;
        }
        int res = cp(cached_copy, filename);
        free(cached_copy);
        chown(filename, uid, gid);
        chmod(filename, mode);
        struct utimbuf ut = {
            .actime = timestamp,
            .modtime = timestamp,
        };
        utime(filename, &ut);
        if (res != 0) {
            ERROR("Failed to write output %s\n", filename);
            goto fail;
        }
    }
    r = NULL;

    return 0;

fail:
    if (r != NULL)
        rowset_discard(r);
    return -1;
}

int cache_close(cache_t *cache) {
    assert(cache != NULL);
    if (db_close(cache->db) != 0)
        return -1;
    free(cache);
    return 0;
}
