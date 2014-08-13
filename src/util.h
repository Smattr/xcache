#ifndef _XCACHE_UTIL_H_
#define _XCACHE_UTIL_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

char *abspath(char *relpath);

/* A version of sprintf that does allocation internally for convenience. */
char *aprintf(const char *fmt, ...);

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

/* Equivalent of `mkdir -p`.
 *
 * path - An absolute or relative path to the final directory to create.
 *
 * Returns 0 on success, -1 on failure.
 */
int mkdirp(const char *path);

/* Equivalent of `du -cS`.
 * 
 * path - An absolute or relative path to a directory whose contents to
 *   measure. Note that subdirectories and special files are not taken into
 *   account.
 *
 * Return -1 on failure or the total size of files in the directory on success.
 */
ssize_t du(const char *path);

/* Remove files within a directory up to a certain limit.
 *
 * path - An absolute or relative path to the directory to prune entries from.
 * reduction - Remove files until we have removed at least this many bytes.
 *
 * Returns the number of bytes that were removed.
 */
size_t reduce(const char *path, size_t reduction);

/* fgets-/getdelim-alike. It's possible I'm too tired or dense at this point,
 * but all the POSIX functions I'm aware of seem to have some deal-breaking
 * flaw for reading a '\0'-terminated string into a static buffer. Perhaps
 * reading programmatic strings from a file descriptor is not a common task.
 */
bool get(char *buffer, size_t limit, FILE *f);

typedef struct file_iter file_iter_t;
file_iter_t *file_iter(const char *path);
void file_iter_destroy(file_iter_t *fi);
char *file_iter_next(file_iter_t *fi);

#endif
