#include <assert.h>
#include "collection/dict.h"
#include "constants.h"
#include "depset.h"
#include "set.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static void *stamp(const char *key) {
    struct stat buf;
    int r = stat(key, &buf);
    if (r == 0)
        return (void*)buf.st_mtime;
    return (void*)MISSING;
}

struct depset {
    dict_t *inputs;
    set_t *outputs;
};

depset_t *depset_new(void) {
    depset_t *o = (depset_t*)malloc(sizeof(depset_t));
    if (o == NULL)
        return NULL;
    o->inputs = dict_new(stamp);
    if (o->inputs == NULL) {
        free(o);
        return NULL;
    }
    o->outputs = set_new();
    if (o->outputs == NULL) {
        dict_destroy(o->inputs);
        free(o);
        return NULL;
    }
    return o;
}

int depset_add_input(depset_t *d, char *filename) {
    if (set_contains(d->outputs, filename))
        /* This is already marked as an output; i.e. we are reading back in
         * something we effectively already know. No need to track this.
         */
        return 0;
    return dict_add(d->inputs, filename);
}

dict_iter_t *depset_iter_inputs(depset_t *d) {
    return dict_iter(d->inputs);
}

int depset_add_output(depset_t *d, char *filename) {
    return set_add(d->outputs, filename);
}

set_iter_t *depset_iter_outputs(depset_t *d) {
    return set_iter(d->outputs);
}

void depset_destroy(depset_t *d) {
    dict_destroy(d->inputs);
    set_destroy(d->outputs);
    free(d);
}
