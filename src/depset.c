#include <assert.h>
#include "collection/dict.h"
#include "constants.h"
#include "depset.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

struct depset {
    dict_t files;
};

depset_t *depset_new(void) {
    depset_t *o = malloc(sizeof(*o));
    if (o == NULL)
        return NULL;

    if (dict(&o->files) != 0) {
        free(o);
        return NULL;
    }

    return o;
}

typedef struct {
    filetype_t type;
    time_t mtime;
} entry_t;

int depset_add(depset_t *d, char *filename, filetype_t type) {
    assert(type == XC_INPUT || type == XC_OUTPUT);
    entry_t *e = dict_lookup(&d->files, filename);
    if (e == NULL) {
        /* We've never seen this item before. */
        e = malloc(sizeof(*e));
        if (e == NULL)
            return -1;
        e->type = type;
        if (type == XC_INPUT) {
            /* We need to measure this file now. */
            struct stat buf;
            int r = stat(filename, &buf);
            e->mtime = r == 0 ? buf.st_mtime : MISSING;
        }
        if (dict_add(&d->files, filename, e) != 0) {
            free(e);
            return -1;
        }
    } else if (e->type == XC_INPUT && type == XC_OUTPUT) {
        /* Writing to something we previously read from. */
        e->type = XC_BOTH;
    }
    return 0;
}

int depset_foreach(depset_t *d, int (*f)(const char *filename, filetype_t type, time_t mtime)) {
    int wrapper(const char *filename, void *value) {
        entry_t *e = value;
        return f(filename, e->type, e->mtime);
    }
    return dict_foreach(&d->files, wrapper);
}

void depset_destroy(depset_t *d) {
    dict_destroy(&d->files);
    free(d);
}
