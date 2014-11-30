#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include "spipe.h"
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "tee.h"
#include <unistd.h>

static char *tee(spipe_t *pipe) {
    char *name = strdup("/tmp/tmp.XXXXXX");
    if (name == NULL) {
        free(pipe);
        return NULL;
    }

    int f = mkstemp(name);
    if (f == -1) {
        free(name);
        free(pipe);
        return NULL;
    }

    do {
        const size_t sz = 1024; /* bytes */
        char buf[sz];
        ssize_t len = spipe_read(pipe, buf, sz);
        if (len == -1)
            goto error;
        else if (len == 0)
            break;

        assert((size_t)len <= sz);
        ssize_t written = write(f, buf, len);
        if (written < len)
            goto error;
        written = spipe_write(pipe, buf, len);
        if (written < len)
            goto error;
    } while (1);

    close(f);
    free(pipe);
    return name;

error:
    close(f);
    unlink(name);
    free(name);
    free(pipe);
    return NULL;
}

struct tee {
    pthread_t thread;
    int sigfd;
};

tee_t *tee_create(int *output) {
    tee_t *t = malloc(sizeof(*t));
    if (t == NULL)
        return NULL;

    int p[2];
    int r = pipe(p);
    if (r != 0) {
        free(t);
        return NULL;
    }

    int q[2];
    r = pipe(q);
    if (r != 0) {
        close(p[0]);
        close(p[1]);
        free(t);
        return NULL;
    }

    spipe_t *s = malloc(sizeof(*s));
    if (s == NULL) {
        close(q[0]);
        close(q[1]);
        close(p[0]);
        close(p[1]);
        free(t);
        return NULL;
    }
    s->in = p[0];
    s->out = *output;
    *output = p[1];

    s->sig = q[0];
    t->sigfd = q[1];
    r = pthread_create(&t->thread, NULL, (void*(*)(void*))tee, s);
    if (r != 0) {
        free(s);
        close(q[0]);
        close(q[1]);
        close(p[0]);
        close(p[1]);
        free(t);
        return NULL;
    }

    return t;
}

char *tee_close(tee_t *t) {
    char c = (char)0; /* <-- irrelevant */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(t->sigfd, &c, 1);
#pragma GCC diagnostic pop
    close(t->sigfd);
    char *ret;
    int r = pthread_join(t->thread, (void**)&ret);
    free(t);
    return r == 0 ? ret : NULL;
}
