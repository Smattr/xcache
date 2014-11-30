#include <assert.h>
#include "spipe.h"
#include <sys/select.h>
#include <unistd.h>

ssize_t spipe_read(spipe_t *sp, void *buf, size_t count) {
    /* Construct the read FD mask. */
    fd_set fs;
    FD_ZERO(&fs);
    FD_SET(sp->sig, &fs);
    FD_SET(sp->in, &fs);
    const int nfds = (sp->in > sp->sig ? sp->in : sp->sig) + 1;

    int selected = select(nfds, &fs, NULL, NULL, NULL);
    if (selected == -1)
        return -1;

    if (FD_ISSET(sp->sig, &fs))
        return 0;

    assert(FD_ISSET(sp->in, &fs));
    return read(sp->in, buf, count);
}

ssize_t spipe_write(spipe_t *sp, void *buf, size_t count) {
    return write(sp->out, buf, count);
}
