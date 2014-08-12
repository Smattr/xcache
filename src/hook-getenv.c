/* A library for hooking getenv and conveying any accesses to Xcache. Note that
 * this file is *NOT* compiled into xcache; it is compiled to a separate
 & library.
 *
 * The expected use case of this is that Xcache is trying to trace a target and
 * wants to, in addition to observing the target's syscalls, observe the
 * target's access to environment variables. In order to do this, Xcache can
 * LD_PRELOAD this library into the target. Note that communication via the
 * 'out' file needs to be cooperative, in the sense that this library and
 * Xcache need to speak the same protocol.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef XCACHE_FILENO
    #error XCACHE_FILENO not defined
#endif

static char *(*real_getenv)(const char *name);
static FILE *out;

static void init(void) {
    assert(real_getenv == NULL);

    /* Locate the libc getenv. */
    real_getenv = dlsym(RTLD_NEXT, "getenv");
    assert(real_getenv != NULL);

    if (fcntl(XCACHE_FILENO, F_GETFD) == -1)
        /* A return pipe back to Xcache doesn't seem to exist. Oh well. */
        return;

    out = fdopen(XCACHE_FILENO, "w");
    assert(out != NULL);
}

/* Write the given (possibly NULL) string to the Xcache pipe. Note that we must
 * follow the protocol Xcache is expecting for conveying string data.
 */
static void output(const char *data) {
    if (out == NULL)
        /* We don't have a pipe to communicate with. */
        return;
    fputc(data == NULL ? 0 : 1, out);
    if (data != NULL)
        fputs(data, out);
    fputc(0, out);
}

/* Hooked version of getenv. We lookup environment variables, as expected, but
 * we also pass any accessed environment variables to Xcache, that we are
 * expecting to be tracing us.
 */
char *getenv(const char *name) {
    if (real_getenv == NULL)
        init();
    assert(real_getenv != NULL);
    output(name);
    char *v = real_getenv(name);
    output(v);
    return v;
}
