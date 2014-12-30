#include <assert.h>
#include "message-protocol.h"
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

        message_t *message = read_message(m->in);
        if (message == NULL)
            goto error;
        if (message->tag != MSG_GETENV) {
            /* Received a call we don't handle. */
            free(message);
            goto error;
        }

        /* FIXME: cope with NULL key or value below */

        if (dict_contains(d, message->key)) {
            free(message->key);
            free(message->value);
        } else {
            if (dict_add(d, message->key, message->value) != 0) {
                free(message->key);
                free(message->value);
                free(message);
                goto error;
            }
        }
        free(message);
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
