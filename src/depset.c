#include <assert.h>
#include "depset.h"
#include "dict.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static time_t unset = -1;
static void *stamp(char *key, void *value) {
    if ((time_t)value == unset) {
        struct stat buf;
        int r = stat(key, &buf);
        if (r == 0) {
            return (void*)buf.st_mtime;
        }
    }
    return value;
}

struct depset {
    dict_t *inputs;
    dict_t *outputs;
    dict_t *missing;
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
    o->outputs = dict_new();
    if (o->outputs == NULL) {
        dict_destroy(o->inputs);
        free(o);
        return NULL;
    }
    o->missing = dict_new();
    if (o->missing == NULL) {
        dict_destroy(o->inputs);
        dict_destroy(o->outputs);
        free(o);
        return NULL;
    }
    return o;
}

int depset_add_input(depset_t *oper, char *filename) {
    assert(oper != NULL);
    return dict_add_if(oper->inputs, filename, (void*)unset, NULL, stamp);
}

int depset_add_output(depset_t *oper, char *filename) {
    assert(oper != NULL);
    return dict_add(oper->outputs, filename, (void*)unset, NULL);
}

int depset_add_missing(depset_t *oper, char *filename) {
    assert(oper != NULL);
    return dict_add(oper->missing, filename, (void*)1, NULL);
}

void depset_destroy(depset_t *oper) {
    assert(oper != NULL);
    dict_destroy(oper->inputs);
    dict_destroy(oper->outputs);
    dict_destroy(oper->missing);
    free(oper);
}
