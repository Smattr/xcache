/* Communication protocol for passing data via a file descriptor. Basically
 * serialisation and deserialisation of strings. Not specific to xcache.
 */

#ifndef _XCACHE_COMM_PROTOCOL_H_
#define _XCACHE_COMM_PROTOCOL_H_

/* Read string data into the pointer 'data'. Returns 0 on success, 1 on EOF of
 * the file descriptor, -1 on error.
 */
int read_string(int fd, char **data);

/* Write string data from the pointer 'data'. Returns 0 on success, -1 on
 * error.
 */
int write_string(int fd, const char *data);

#endif
