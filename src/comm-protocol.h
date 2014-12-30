/* Communication protocol for passing data via a file descriptor. Basically
 * serialisation and deserialisation of strings. Not specific to xcache.
 */

#ifndef _XCACHE_COMM_PROTOCOL_H_
#define _XCACHE_COMM_PROTOCOL_H_

#include <stdlib.h>

/* Read string data into the pointer 'data'. Returns 0 on success, 1 on EOF of
 * the file descriptor, -1 on error.
 */
ssize_t read_data(int fd, unsigned char **data);

/* Write string data from the pointer 'data'. Returns 0 on success, -1 on
 * error.
 */
int write_data(int fd, const unsigned char *data, size_t len);

#endif
