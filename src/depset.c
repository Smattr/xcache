#include <assert.h>
#include "constants.h"
#include "depset.h"
#include "dict.h"
#include "set.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static void *stamp(char *key, void *value) {
    if ((time_t)value == UNSET) {
        struct stat buf;
        int r = stat(key, &buf);
        if (r == 0) {
            return (void*)buf.st_mtime;
        }
        return (void*)MISSING;
    }
    return value;
}

struct depset {
    dict_t *inputs;
    set_t *outputs;
};

depset_t *depset_new(void) {
    depset_t *o = (depset_t*)malloc(sizeof(depset_t));
    if (o == NULL) {
        return NULL;
    }
    o->inputs = dict_new();
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

int depset_add_input(depset_t *oper, char *filename) {
    assert(oper != NULL);
    if (set_contains(oper->outputs, filename)) {
        /* This is already marked as an output; i.e. we are reading back in
         * something we effectively already know. No need to track this.
         */
        return 0;
    }
    return dict_add_if(oper->inputs, filename, stamp);
}

iter_t *depset_iter_inputs(depset_t *oper) {
    return dict_iter(oper->inputs);
}

int depset_add_output(depset_t *oper, char *filename) {
    assert(oper != NULL);
    return set_add(oper->outputs, filename);
}

set_iter_t *depset_iter_outputs(depset_t *oper) {
    return set_iter(oper->outputs);
}

void depset_destroy(depset_t *oper) {
    assert(oper != NULL);
    dict_destroy(oper->inputs);
    set_destroy(oper->outputs);
    free(oper);
}
