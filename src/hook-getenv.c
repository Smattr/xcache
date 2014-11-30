/* A library for hooking getenv and conveying any accesses to Xcache. Note that
 * this file is *NOT* compiled into xcache; it is compiled into a separate
 * library.
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

static char *(*real_getenv)(const char *name);
static int out = -1;

static void init(void) {
    assert(real_getenv == NULL);

    /* Locate the libc getenv. */
    real_getenv = dlsym(RTLD_NEXT, "getenv");
    assert(real_getenv != NULL);

    char *xcache_pipe = real_getenv(XCACHE_PIPE);
    if (xcache_pipe == NULL)
        /* No return pipe available. */
        return;

    char *end;
    int fd = strtol(xcache_pipe, &end, 10);
    if (*end != '\0')
        /* The string wasn't entirely an integer. */
        return;

    if (fcntl(fd, F_GETFD) == -1)
        /* The return pipe is not valid. */
        return;

    /* We received a valid return pipe. */
    out = fd;
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
    if (out != -1) {
        write_string(out, "getenv");
        write_string(out, name);
        write_string(out, v);
    }
    return v;
}
