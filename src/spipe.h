#ifndef _XCACHE_SPIPE_H_
#define _XCACHE_SPIPE_H_

#include <unistd.h>

/* Equivalent of an in-memory pipe, but with an extra input for a third-party
 * to signal that the pipe should be closed.
 */
typedef struct {
    int sig;
    int in;
    int out;
} spipe_t;

/* Read from the pipe.
 *
 * sp - The pipe to read from.
 * buf - Buffer to read into.
 * count - Bytes to read.
 *
 * Returns -1 on error, 0 on signal received, otherwise the number of bytes
 * read.
 */
ssize_t spipe_read(spipe_t *sp, void *buf, size_t count);

/* Write to the pipe.
 *
 * sp - The pipe to write to.
 * buf - Buffer to write from.
 * count - Bytes to write.
 *
 * Returns number of bytes written or -1 on error.
 */
ssize_t spipe_write(spipe_t *sp, void *buf, size_t count);

#endif
