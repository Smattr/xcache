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
    dict_t *env;
} hook_arg_t;

static void hook(hook_arg_t *args) {
    while (true) {
        fd_set fs;
        FD_ZERO(&fs);
        FD_SET(args->sig, &fs);
        FD_SET(args->in, &fs);
        const int nfds = (args->in > args->sig ? args->in : args->sig) + 1;

        int selected = select(nfds, &fs, NULL, NULL, NULL);
        if (selected == -1) {
            free(args);
            return;
        }

        if (FD_ISSET(args->sig, &fs)) {
            free(args);
            return;
        }

        assert(FD_ISSET(args->in, &fs));

        message_t *message = read_message(args->in);
        if (message == NULL) {
            free(args);
            return;
        }
        if (message->tag != MSG_GETENV) {
            /* Received a call we don't handle. */
            free(message);
            free(args);
            return;
        }

        /* FIXME: cope with NULL key or value below */

        if (dict_contains(args->env, message->key)) {
            free(message->key);
            free(message->value);
        } else {
            if (dict_add(args->env, message->key, message->value) != 0) {
                free(message->key);
                free(message->value);
                free(message);
                free(args);
                return;
            }
        }
        free(message);
    }

    assert(!"unreachable");
}

hook_t *hook_create(int input, dict_t *env) {
    hook_t *h = malloc(sizeof(*h));
    if (h == NULL)
        return NULL;

    hook_arg_t *args = malloc(sizeof(*args));
    if (args == NULL) {
        free(h);
        return NULL;
    }
    args->in = input;
    args->env = env;

    int p[2];
    if (pipe(p) != 0) {
        free(args);
        free(h);
        return NULL;
    }
    args->sig = p[0];
    h->sigfd = p[1];

    if (pthread_create(&h->thread, NULL, (void*(*)(void*))hook, args) != 0) {
        close(p[0]);
        close(p[1]);
        free(args);
        free(h);
        return NULL;
    }

    return h;
}

int hook_close(hook_t *h) {
    char c = (char)0; /* <-- irrelevant */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(h->sigfd, &c, 1);
#pragma GCC diagnostic pop
    close(h->sigfd);
    int r = pthread_join(h->thread, NULL);
    free(h);
    return r;
}
