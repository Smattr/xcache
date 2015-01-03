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
#include <fcntl.h>
#include "message-protocol.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

static char *internal_getenv(const char *name) {
    if (name == NULL) {
        /* I don't think it is legal to pass NULL to getenv, but let's allow it
         * anyway.
         */
        return NULL;
    }

    size_t len = strlen(name);

    for (char **p = environ; *p != NULL; p++) {
        if (strncmp(name, *p, len) == 0 && strlen(*p) >= len + 1
                && (*p)[len] == '=')
            return *p + len + 1;
    }

    /* Didn't find the given variable. */
    return NULL;
}

static int out_pipe(void) {
    static bool initialised = false;
    static int out_fd = -1;

    if (!initialised) {
        initialised = true;

        char *xcache_pipe = internal_getenv(XCACHE_PIPE);
        if (xcache_pipe == NULL) {
            /* No return pipe available. */
            return -1;
        }

        char *end;
        int fd = strtol(xcache_pipe, &end, 10);
        if (*end != '\0') {
            /* The string wasn't entirely an integer. */
            return -1;
        }

        if (fcntl(fd, F_GETFD) == -1) {
            /* The return pipe is not valid. */
            return -1;
        }

        /* We received a valid return pipe. */
        out_fd = fd;
    }

    return out_fd;
}

/* Hooked version of getenv. We lookup environment variables, as expected, but
 * we also pass any accessed environment variables to Xcache, that we are
 * expecting to be tracing us.
 */
char *getenv(const char *name) {
    char *v = internal_getenv(name);

    if (name == NULL)
        return v;

    int out_fd = out_pipe();

    /* Lock the message pipe back to xcache in case our host is multithreaded
     * and we end up with two threads in this function concurrently. This
     * actually should not happen because it is undefined to call getenv in a
     * multithreaded program.
     */
    if (out_fd != -1 && flock(out_fd, LOCK_EX) == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        /* For some reason, GCC doesn't seem to notice that we *are*
         * initialising 'value' here.
         */
        message_t message = {
            .tag = MSG_GETENV,
            .key = (char*)name,
            .value = v,
        };
#pragma GCC diagnostic pop
        write_message(out_fd, &message);
        flock(out_fd, LOCK_UN);
    }

    return v;
}
