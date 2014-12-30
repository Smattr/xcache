#include <assert.h>
#include "comm-protocol.h"
#include "message-protocol.h"
#include <stdlib.h>
#include <string.h>

/* Read a MSG_GETENV message. */
static int read_getenv(int fd, message_t *message) {
    assert(message != NULL);

    message->tag = MSG_GETENV;

    if (read_data(fd, (unsigned char**)&message->key) < 0)
        return -1;

    if (read_data(fd, (unsigned char**)&message->value) < 0)
        return -1;

    return 0;
}

/* Read a MSG_EXEC_ERROR message. */
static int read_exec_error(int fd, message_t *message) {
    assert(message != NULL);

    message->tag = MSG_EXEC_ERROR;

    unsigned char *data;
    ssize_t len = read_data(fd, &data);
    if (len != sizeof(message->errnumber)) {
        if (len > 0)
            free(data);
        return -1;
    }
    memcpy(&message->errnumber, data, sizeof(message->errnumber));
    free(data);

    return 0;
}

message_t *read_message(int fd) {
    /* Read the message tag. */
    unsigned char *data;
    ssize_t len = read_data(fd, &data);
    if (len != sizeof(message_tag_t))
        return NULL;
    message_tag_t *tag = (message_tag_t*)data;

    message_t *message = malloc(sizeof(*message));
    if (message == NULL)
        return NULL;

    switch (*tag) {
        case MSG_GETENV:
            if (read_getenv(fd, message) != 0) {
                free(message);
                return NULL;
            }
            break;

        case MSG_EXEC_ERROR:
            if (read_exec_error(fd, message) != 0) {
                free(message);
                return NULL;
            }
            break;

        default:
            /* Unknown tag. */
            free(message);
            return NULL;
    }

    return message;
}

/* Write a MSG_GETENV message. */
static int write_getenv(int fd, message_t *message) {
    assert(message->tag == MSG_GETENV);

    if (write_data(fd, (unsigned char*)message->key,
            message->key == NULL ? 0 : strlen(message->key) + 1) != 0)
        return -1;

    if (write_data(fd, (unsigned char*)message->value,
            message->value == NULL ? 0 : strlen(message->value) + 1) != 0)
        return -1;

    return 0;
}

/* Write a MSG_EXEC_ERROR message. */
static int write_exec_error(int fd, message_t *message) {
    assert(message->tag == MSG_EXEC_ERROR);

    if (write_data(fd, (unsigned char*)&message->errnumber,
            sizeof(message->errnumber)) != 0)
        return -1;

    return 0;
}

int write_message(int fd, message_t *message) {
    if (write_data(fd, (unsigned char*)&message->tag, sizeof(message->tag)) < 0)
        return -1;

    switch (message->tag) {
        case MSG_GETENV:
            return write_getenv(fd, message);

        case MSG_EXEC_ERROR:
            return write_exec_error(fd, message);

        default:
            return -1;
    }
}
