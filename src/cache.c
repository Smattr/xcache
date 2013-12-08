#include <assert.h>
#include "cache.h"
#include "queries.h"
#include <stdlib.h>
#include <sqlite3.h>

struct cache {
    sqlite3 *db;
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
    return c;
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
