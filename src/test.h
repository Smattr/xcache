/* Test suite infrastructure. */

#ifndef _XCACHE_TEST_H_
#define _XCACHE_TEST_H_

/* The build system defines this variable when building the test suite. */
#ifdef XCACHE_TEST

/* Register a new unit test with the given (human readable) name. The test
 * logic itself should be in the function passed as the second parameter. This
 * function should not be called directly, but should just be invoked through
 * the TEST macro below.
 */
void register_test(const char *name, int (*fn)(void));

#define test_assert(expr) \
    do { \
        if (!(expr)) { \
            return -1; \
        } \
    } while (0)

/* Keyword to be used on static functions. This allows us to write out-of-line
 * tests, even for static functions, and have them be compiled as non-static in
 * the test suite.
 */
#define STATIC /* nothing */

/* Declare a new unit test. Usage should be as if you were defining a function:
 *  TEST(foo) {
 *    ...
 *  }
 */
#define TEST(fn) \
    static int testcase_##fn(void); \
    static void __attribute__((constructor(200 + __COUNTER__))) _testcase_##fn(void) { \
        register_test(#fn, testcase_##fn); \
    } \
    static int testcase_##fn(void)

#else

/* Equivalents in the case when we are not running the test suite. */
#define register_test(fn) /* nothing */
#define test_assert(expr) \
    do { \
        if (!(expr)) { \
            /* do nothing */ \
        } \
    } while (0)
#define STATIC static
#define TEST(fn) static int __attribute__((unused)) testcase_##fn(void)

#endif

#endif
