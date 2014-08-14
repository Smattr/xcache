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
#include "comm-protocol.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef XCACHE_FILENO
    #error XCACHE_FILENO not defined
#endif

static char *(*real_getenv)(const char *name);
static int out;

static void init(void) {
    assert(real_getenv == NULL);

    /* Locate the libc getenv. */
    real_getenv = dlsym(RTLD_NEXT, "getenv");
    assert(real_getenv != NULL);

    if (fcntl(XCACHE_FILENO, F_GETFD) != -1)
        /* The return pipe is valid. */
        out = XCACHE_FILENO;
}

/* Hooked version of getenv. We lookup environment variables, as expected, but
 * we also pass any accessed environment variables to Xcache, that we are
 * expecting to be tracing us.
 */
char *getenv(const char *name) {
    if (real_getenv == NULL)
        init();
    assert(real_getenv != NULL);
    char *v = real_getenv(name);
    if (out != 0) {
        write_string(out, name);
        write_string(out, v);
    }
    return v;
}
