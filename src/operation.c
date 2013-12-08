#include <assert.h>
#include "dict.h"
#include "operation.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static time_t unset = -1;
static int is_unset(void *value) {
    return (time_t)value == unset;
}

struct operation {
    dict_t *inputs;
    dict_t *outputs;
    dict_t *missing;
};

operation_t *operation_new(void) {
    operation_t *o = (operation_t*)malloc(sizeof(operation_t));
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

int operation_add_input(operation_t *oper, char *filename) {
    assert(oper != NULL);
    return dict_add_if(oper->inputs, filename, (void*)unset, NULL, is_unset);
}

int operation_add_output(operation_t *oper, char *filename) {
    assert(oper != NULL);
    return dict_add(oper->outputs, filename, (void*)unset, NULL);
}

int operation_add_missing(operation_t *oper, char *filename) {
    assert(oper != NULL);
    return dict_add(oper->missing, filename, (void*)1, NULL);
}

void operation_destroy(operation_t *oper) {
    assert(oper != NULL);
    dict_destroy(oper->inputs);
    dict_destroy(oper->outputs);
    dict_destroy(oper->missing);
    free(oper);
}
