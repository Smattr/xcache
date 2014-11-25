/* Test suite infrastructure. */

#ifndef _XCACHE_TEST_H_
#define _XCACHE_TEST_H_

/* The build system defines this variable when building the test suite. */
#ifdef XCACHE_TEST

/* Keyword to be used on static functions. This allows us to write out-of-line
 * tests, even for static functions, and have them be compiled as non-static in
 * the test suite.
 */
#define STATIC /* nothing */

#else

#define STATIC static

#endif

#endif
