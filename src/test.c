/* Force assertions on. */
#ifdef NDEBUG
    #undef NDEBUG
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

#ifndef XCACHE_TEST
    #error This file is not intended to be included in anything other than the test suite
#endif

/* A linked list to contain the tests. Note, we don't want to use the linked
 * list implementation in utils in case it has bugs, which we may be testing
 * for.
 */
typedef struct _node {
    const char *name;
    int (*fn)(void);
    struct _node *next;
} node_t;
static node_t *tests;

void register_test(const char *name, int (*fn)(void)) {
    node_t *n = malloc(sizeof(*n));
    assert(n != NULL);
    n->name = name;
    n->fn = fn;
    n->next = tests;
    tests = n;
}

int main(int argc, char **argv) {
    printf("Starting xcache test suite...\n");
    int total = 0;

    /* Loop through all the registered tests. */
    for (node_t *n = tests; n != NULL; n = n->next) {

        /* Let the user run specific tests by passing a command line list of
         * them.
         */
        if (argc > 1) {
            bool enabled = false;
            for (int i = 1; i < argc; i++) {
                if (!strcmp(argv[i], n->name)) {
                    enabled = true;
                    break;
                }
            }
            if (!enabled) {
                continue;
            }
        }

        /* Run the test and report the result. */
        printf(" %s...", n->name);
        int result = n->fn();
        if (result == 0) {
            printf("OK\n");
        } else {
            printf("Failed\n");
            total++;
        }
    }

    /* Return an exit status representing the test suite result. */
    if (total > 0) {
        printf("%d tests failed\n", total);
        return -1;
    }
    printf("All tests succeeded\n");
    return 0;
}
