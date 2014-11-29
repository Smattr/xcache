#include <assert.h>
#include "collection/dict.h"
#include "collection/set.h"
#include "constants.h"
#include "depset.h"
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
    dict_t inputs;
    set_t outputs;
};

depset_t *depset_new(void) {
    depset_t *o = malloc(sizeof(*o));
    if (o == NULL)
        goto fail1;
    if (dict(&o->inputs, stamp) != 0)
        goto fail2;
    if (set(&o->outputs) != 0)
        goto fail3;
    return o;

fail3: set_destroy(&o->outputs);
fail2: dict_destroy(&o->inputs);
fail1: free(o);
    return NULL;
}

int depset_add_input(depset_t *d, char *filename) {
    if (set_contains(&d->outputs, filename))
        /* This is already marked as an output; i.e. we are reading back in
         * something we effectively already know. No need to track this.
         */
        return 0;

    if (dict_contains(&d->inputs, filename))
        /* Avoid adding an input if we have already tracked it or we will end
         * up re-measuring it and overwriting the original stat data.
         */
        return 0;
    return dict_add(&d->inputs, filename, NULL);
}

int depset_foreach_input(depset_t *d, int (*f)(const char *filename, time_t mtime)) {
    int wrapper(const char *filename, void *value) {
        return f(filename, (time_t)value);
    }
    return dict_foreach(&d->inputs, wrapper);
}

int depset_add_output(depset_t *d, char *filename) {
    return set_add(&d->outputs, filename);
}

int depset_foreach_output(depset_t *d, int (*f)(const char *filename)) {
    return set_foreach(&d->outputs, f);
}

void depset_destroy(depset_t *d) {
    dict_destroy(&d->inputs);
    set_destroy(&d->outputs);
    free(d);
}
