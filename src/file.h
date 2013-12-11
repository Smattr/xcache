#ifndef _XCACHE_FILE_H_
#define _XCACHE_FILE_H_

/* Return the hash of the contents of a file. The caller should not rely on any
 * property of the hash except it being deterministic and printable. This
 * includes that the caller should not assume a particular hashing algorithm is
 * in use.
 *
 * filename - An absolute path to the file to hash.
 *
 * Returns a pointer to the hash or NULL on failure. It is the caller's
 * responsibility to free the returned pointer.
 */
char *filehash(const char *filename);

/* Copy a file, preserving the permissions, owner and group if possible.
 *
 * from - Absolute path of source.
 * to - Absolute path of destination.
 *
 * Returns 0 on success, -1 on failure.
 */
int cp(const char *from, const char *to);

#endif
