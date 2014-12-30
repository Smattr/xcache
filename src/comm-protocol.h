/* Communication protocol for passing data via a file descriptor. Basically
 * serialisation and deserialisation. Not specific to xcache.
 */

#ifndef _XCACHE_COMM_PROTOCOL_H_
#define _XCACHE_COMM_PROTOCOL_H_

#include <stdlib.h>

/* Read into the pointer 'data'. Returns the number of bytes read, -1 on error,
 * -2 on EOF.
 */
ssize_t read_data(int fd, unsigned char **data);

/* Write from the pointer 'data'. Returns 0 on success, -1 on error. */
int write_data(int fd, const unsigned char *data, size_t len);

#endif
