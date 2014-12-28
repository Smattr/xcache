#include <assert.h>
#include "collection/dict.h"
#include "constants.h"
#include "depset.h"
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

struct depset {
    dict_t files;
#ifndef NDEBUG
    bool finalised;
#endif
};

depset_t *depset_new(void) {
    depset_t *o = malloc(sizeof(*o));
    if (o == NULL)
        return NULL;

    if (dict(&o->files) != 0) {
        free(o);
        return NULL;
    }

#ifndef NDEBUG
    o->finalised = false;
#endif

    return o;
}

typedef struct {
    filetype_t type;
    time_t mtime;
} entry_t;

int depset_add(depset_t *d, char *filename, filetype_t type) {
    assert(!d->finalised);

    if (type == XC_BOTH) {
        /* A file should only ever claimed to be a single role by a caller. */
        return -1;
    }

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
        char *name = strdup(filename);
        if (name == NULL) {
            free(e);
            return -1;
        }
        if (dict_add(&d->files, name, e) != 0) {
            free(name);
            free(e);
            return -1;
        }
    } else if (e->type == XC_INPUT && type == XC_OUTPUT) {
        /* Writing to something we previously read from. */
        e->type = XC_BOTH;
    } else if (e->type == XC_AMBIGUOUS) {
        /* If the type of this item was previously ambiguous, we may have just
         * clarified its ambiguity. Note that this is a no-op if the caller has
         * still claimed this item is ambiguous.
         */
        e->type = type;
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

int depset_finalise(depset_t *d) {
    assert(!d->finalised);
    int finalise(const char *_ __attribute__((unused)), void *value) {
        entry_t *e = value;
        if (e->type == XC_AMBIGUOUS) {
            /* If an item was ambiguous and we have seen no evidence that it
             * was an output, we can now consider it an input.
             */
            e->type = XC_INPUT;
        }
        return 0;
    }
    int result = dict_foreach(&d->files, finalise);
#ifndef NDEBUG
    if (result == 0) {
        d->finalised = true;
    }
#endif
    return result;
}

void depset_destroy(depset_t *d) {
    dict_destroy(&d->files);
    free(d);
}
