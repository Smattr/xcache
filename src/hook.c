#include <assert.h>
#include "comm-protocol.h"
#include "collection/dict.h"
#include "hook.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

typedef struct {
    int sig;
    int in;
} mux_t;

static dict_t *hook(mux_t *m) {
    dict_t *d = calloc(1, sizeof(*d));
    if (d == NULL)
        goto error;
    if (dict(d) != 0)
        goto error;

    while (true) {
        fd_set fs;
        FD_ZERO(&fs);
        FD_SET(m->sig, &fs);
        FD_SET(m->in, &fs);
        const int nfds = (m->in > m->sig ? m->in : m->sig) + 1;

        int selected = select(nfds, &fs, NULL, NULL, NULL);
        if (selected == -1)
            goto error;

        if (FD_ISSET(m->sig, &fs)) {
            free(m);
            return d;
        }

        assert(FD_ISSET(m->in, &fs));

        char *call;
        ssize_t len = read_data(m->in, (unsigned char**)&call);
        if (len < 0)
            goto error;
        if (call == NULL || strcmp(call, "getenv") != 0) {
            /* Received a call we don't handle. */
            free(call);
            goto error;
        }
        free(call);

        /* FIXME: cope with NULL key or value below */

        char *key;
        len = read_data(m->in, (unsigned char**)&key);
        if (len < 0)
            goto error;

        char *value;
        len = read_data(m->in, (unsigned char**)&value);
        if (len < 0) {
            free(key);
            goto error;
        }
        if (dict_contains(d, key)) {
            free(key);
            free(value);
        } else {
            if (dict_add(d, key, value) != 0) {
                free(key);
                free(value);
                goto error;
            }
        }
    }

    assert(!"unreachable");
    return NULL;

error:
    if (d != NULL) {
        if (d->table != NULL)
            dict_destroy(d);
        free(d);
    }
    free(m);
    return NULL;
}

hook_t *hook_create(int input) {
    hook_t *h = malloc(sizeof(*h));
    if (h == NULL)
        return NULL;

    mux_t *m = malloc(sizeof(*m));
    if (m == NULL) {
        free(h);
        return NULL;
    }
    m->in = input;

    int p[2];
    if (pipe(p) != 0) {
        free(m);
        free(h);
        return NULL;
    }
    m->sig = p[0];
    h->sigfd = p[1];

    if (pthread_create(&h->thread, NULL, (void*(*)(void*))hook, m) != 0) {
        close(p[0]);
        close(p[1]);
        free(m);
        free(h);
        return NULL;
    }

    return h;
}

dict_t *hook_close(hook_t *h) {
    char c = (char)0; /* <-- irrelevant */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(h->sigfd, &c, 1);
#pragma GCC diagnostic pop
    close(h->sigfd);
    dict_t *ret;
    int r = pthread_join(h->thread, (void**)&ret);
    free(h);
    return r == 0 ? ret : NULL;
}
