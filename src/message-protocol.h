/* Higher level messaging protocol built on top of the low level helpers in
 * comm-protocol.h. Unlike the comm-protocol.h functions, these *are*
 * Xcache-specific.
 */

#ifndef _XCACHE_MESSAGE_PROTOCOL_H_
#define _XCACHE_MESSAGE_PROTOCOL_H_

/* Type of a message. */
typedef enum {
    MSG_GETENV,      /* Call to getenv() */
    MSG_EXEC_ERROR,  /* Error calling execvp() */
} message_tag_t;

/* A message content. */
typedef struct {
    message_tag_t tag;
    union {
        struct /* MSG_GETENV */ {
            char *key;
            char *value;
        };
        struct /* MSG_EXEC_ERROR */ {
            int errnumber;
        };
    };
} message_t;

/* Read a message from a file descriptor. Returns NULL on error. */
message_t *read_message(int fd);

/* Write a message to a file descriptor. Returns non-zero on error. */
int write_message(int fd, message_t *message);

#endif
