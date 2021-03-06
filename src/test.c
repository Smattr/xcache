/* Force assertions on. */
#ifdef NDEBUG
    #undef NDEBUG
#endif

#ifndef XCACHE_TEST
    #error This file is not intended to be included in anything other than the test suite
#endif

#include <assert.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

/* Some functions that are defined elsewhere with the STATIC macro. */
extern const char *atom(char **path);
extern size_t append(char *dest, size_t dest_len, const char *src);

static void test_atom_nothing(void) {
    char *s = strdup("");
    assert(s != NULL);
    const char *a = atom(&s);
    CU_ASSERT_PTR_NULL(a);
}

static void test_atom_slash(void) {
    char *s = strdup("/");
    assert(s != NULL);
    const char *a = atom(&s);
    CU_ASSERT_PTR_NULL(a);
}

static void test_atom_slashslash(void) {
    char *s = strdup("//");
    assert(s != NULL);
    const char *a = atom(&s);
    CU_ASSERT_PTR_NULL(a);
}

static void test_atom_single(void) {
    char *s = strdup("hello");
    assert(s != NULL);
    const char *a = atom(&s);
    CU_ASSERT_STRING_EQUAL(a, "hello");
    a = atom(&s);
    CU_ASSERT_PTR_NULL(a);
}

static void test_atom_single_abs(void) {
    char *s = strdup("/hello");
    assert(s != NULL);
    const char *a = atom(&s);
    CU_ASSERT_STRING_EQUAL(a, "hello");
    a = atom(&s);
    CU_ASSERT_PTR_NULL(a);
}

static void test_atom_single_trailing(void) {
    char *s = strdup("hello/");
    assert(s != NULL);
    const char *a = atom(&s);
    CU_ASSERT_STRING_EQUAL(a, "hello");
    a = atom(&s);
    CU_ASSERT_PTR_NULL(a);
}

static void test_atom_single_abs_trailing(void) {
    char *s = strdup("/hello/");
    assert(s != NULL);
    const char *a = atom(&s);
    CU_ASSERT_STRING_EQUAL(a, "hello");
    a = atom(&s);
    CU_ASSERT_PTR_NULL(a);
}

static void test_append_basic(void) {
    char a[PATH_MAX];
    strcat(a, "/a");
    char *b = "b";
    size_t len = append(a, strlen(a), b);
    CU_ASSERT_EQUAL(len, strlen(a));
    CU_ASSERT_STRING_EQUAL(a, "/a/b");
}

int main(void) {
    printf("Starting xcache test suite...\n");

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CU_ErrorCode err = CU_get_error();
        fprintf(stderr, "failed to initialise CUnit registry\n");
        return err;
    }

    CU_pSuite suite = CU_add_suite("atom", NULL, NULL);
    if (suite == NULL) {
        CU_ErrorCode err = CU_get_error();
        CU_cleanup_registry();
        fprintf(stderr, "failed to add suite\n");
        return err;
    }
    if ((CU_add_test(suite, "atom(\"\")", test_atom_nothing) == NULL) ||
        (CU_add_test(suite, "atom(\"/\")", test_atom_slash) == NULL) ||
        (CU_add_test(suite, "atom(\"//\")", test_atom_slashslash) == NULL) ||
        (CU_add_test(suite, "atom of single word", test_atom_single) == NULL) ||
        (CU_add_test(suite, "atom of top-level directory", test_atom_single_abs) == NULL) ||
        (CU_add_test(suite, "atom of single word with trailing slash", test_atom_single_trailing) == NULL) ||
        (CU_add_test(suite, "atom of top-level directory with trailing slash", test_atom_single_abs_trailing) == NULL) ||
        (CU_add_test(suite, "append(\"/a\", \"b\")", test_append_basic) == NULL)) {
        CU_ErrorCode err = CU_get_error();
        fprintf(stderr, "failed to add tests\n");
        CU_cleanup_registry();
        return err;
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_ErrorCode err = CU_basic_run_tests();

    unsigned int failed = CU_get_number_of_tests_failed();

    CU_cleanup_registry();

    if (err != CUE_SUCCESS) {
        /* This case seems to never be triggered as CU_basic_run_tests always
         * returns success. Bug in CUnit documentation?
         */
        fprintf(stderr, "Test suite failure (%d)\n", err);
        return err;
    }

    return failed;
}
