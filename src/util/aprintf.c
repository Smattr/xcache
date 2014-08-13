/* Convenience wrapper around asprintf. */

#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>

char *aprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *strp;
    int err = vasprintf(&strp, fmt, ap);
    va_end(ap);
    if (err == -1)
        return NULL;
    return strp;
}
