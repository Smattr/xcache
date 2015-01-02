#include <assert.h>
#include "message-protocol.h"
#include "collection/dict.h"
#include "hook.h"
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

/* Non-blocking check of whether a file descriptor is ready to read from. */
static bool ready(int fd) {
    struct pollfd fds[] = {
        {
            .fd = fd,
            .events = POLLIN,
        },
    };
    if (poll(fds, 1, 0) > 1)
        return true;
    return false;
}

/* Drain an input file descriptor and replicate its contents into two output
 * descriptors.
 */
ssize_t tee(int in, int out1, int out2) {
    assert(in >= 0);
    assert(out1 >= 0);
    assert(out2 >= 0);

    /* This function is only expected to be called on descriptors that are
     * ready to be read from.
     */
    assert(ready(in));

    ssize_t total = 0;
    ssize_t len;
    char buffer[1024];
    do {
        len = read(in, buffer, sizeof(buffer));
        assert(len <= sizeof(buffer));

        if (len == -1)
            break;

        /* We ignore the number of bytes written because there's not much we can
         * do about it.
         */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        (void)write(out1, buffer, len);
        (void)write(out2, buffer, len);
#pragma GCC diagnostic pop

        total += len;
        
        /* Loop to make sure we completely drain the input. */
    } while (len == sizeof(buffer) && ready(in));

    return total;
}

/* Monitor (and log) all the relevant file descriptor operations performed by a
 * tracee. This currently covers:
 *  - stdout logging (ala tee)
 *  - stderr logging similarly
 *  - processing messages from the message pipe (predominantly getenv calls)
 * This function also listens for bytes on the signal pipe and takes this as a
 * command to clean up and exit.
 */
static void hook(target_t *target) {
    while (true) {
        /* Setup a mask of all the file descriptors we need to monitor. */
        fd_set fs;
        FD_ZERO(&fs);
        FD_SET(target->stdout_pipe[0], &fs);
        int nfds = target->stdout_pipe[0];
        FD_SET(target->stderr_pipe[0], &fs);
        if (target->stderr_pipe[0] > nfds)
            nfds = target->stderr_pipe[0];
        FD_SET(target->msg_pipe[0], &fs);
        if (target->msg_pipe[0] > nfds)
            nfds = target->msg_pipe[0];
        FD_SET(target->sig_pipe[0], &fs);
        if (target->sig_pipe[0] > nfds)
            nfds = target->sig_pipe[0];
        nfds++;

        int selected = select(nfds, &fs, NULL, NULL, NULL);
        if (selected == -1) {
            /* Select failed. */
            return;
        }

        /* Log stdout data to our temporary file and replicate it on the actual
         * stdout.
         */
        if (FD_ISSET(target->stdout_pipe[0], &fs)) {
            assert(ready(target->stdout_pipe[0]) &&
                "stdout not ready after claiming to be; someone else reading it?");
            (void)tee(target->stdout_pipe[0], target->outfd, STDOUT_FILENO);
        }

        /* As above for stderr. */
        if (FD_ISSET(target->stderr_pipe[0], &fs)) {
            assert(ready(target->stderr_pipe[0]) &&
                "stderr not ready after claiming to be; someone else reading it?");
            (void)tee(target->stderr_pipe[0], target->errfd, STDERR_FILENO);
        }

        /* Handle any messages we received from the tracee. */
        if (FD_ISSET(target->msg_pipe[0], &fs)) {
            assert(ready(target->msg_pipe[0]) &&
                "message pipe not ready after claiming to be; someone else reading it?");
            do {
                message_t *message = read_message(target->msg_pipe[0]);
                if (message == NULL) {
                    /* Failed to read a message. OOM? */
                    return;
                }
                if (message->tag != MSG_GETENV) {
                    /* Received a call we don't handle. */
                    free(message);
                    return;
                }

                /* FIXME: cope with NULL key or value below */

                if (dict_contains(&target->env, message->key)) {
                    free(message->key);
                    free(message->value);
                } else {
                    if (dict_add(&target->env, message->key, message->value) != 0) {
                        free(message->key);
                        free(message->value);
                        free(message);
                        return;
                    }
                }
                free(message);
            } while (ready(target->msg_pipe[0]));
        }

        /* Check whether the main thread has notified us to exit. */
        if (FD_ISSET(target->sig_pipe[0], &fs)) {
            /* We don't actually care about the byte that's in the signal pipe,
             * but weird kernel buffer settings could mean the main thread is
             * actually blocked on its write to the pipe.
             */
            char ignored;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
            (void)read(target->sig_pipe[0], &ignored, 1);
#pragma GCC diagnostic pop

            /* XXX: I don't *think* an fd flush is required here, but should
             * double check this.
             */

            /* Clean exit. */
            return;
        }

    }

    assert(!"unreachable");
}

int hook_create(target_t *target) {
    return pthread_create(&target->hook, NULL, (void*(*)(void*))hook, target);
}

int hook_close(target_t *target) {
    char c = (char)0; /* <-- irrelevant */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(target->sig_pipe[1], &c, 1);
#pragma GCC diagnostic pop
    close(target->sig_pipe[1]);
    return pthread_join(target->hook, NULL);
}
