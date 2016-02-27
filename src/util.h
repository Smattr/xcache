/** \file Various miscellaneous utility functions. None of this is
 * xcache-specific.
 */

#ifndef _XCACHE_UTIL_H_
#define _XCACHE_UTIL_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/** \brief Join two paths and normalise the result.
 *
 * @param dest Path prefix. This is assumed to already be normalised and point
 *   to at least `PATH_MAX` bytes of available memory.
 * @param src Path suffix.
 */
void normpath(char *dest, char *src);

/** \brief Resolve and normalise a path.
 *
 * This does the equivalent of `realpath`, but does not require the resulting
 * path to exist. Note that it does not resolve symlinks.
 *
 * @param cwd Root for the relative path to be resolved from.
 * @param relpath A relative or absolute path to normalise.
 * @return An absolute path or `NULL` on error.
 */
char *abspath(const char *cwd, char *relpath) __attribute__((nonnull));

/** \brief A version of `sprintf` that does allocation internally for
 * convenience.
 *
 * @param fmt printf-style format string.
 * @return The sprintf-ed string or `NULL` on allocation failure.
 */
char *aprintf(const char *fmt, ...);

/** \brief Support for managed pointers.
 *
 * This function is not intended to be called directly, but is expected to be
 * accessed through the adjacent macro. Sample usage:
 *
 *     void foo(void) {
 *       autofree int *x = malloc(sizeof(int));
 *       // no need to call free(x)
 *     }
 *
 * @param p Pointer to a pointer to free.
 */
static inline void autofree_(void *p) {
    void **q = p;
    free(*q);
}
#define autofree __attribute__((cleanup(autofree_)))

/** \brief Return the hash of the contents of a file.
 *
 * The caller should not rely on any property of the hash except it being
 * deterministic and printable. This includes that the caller should not assume
 * a particular hashing algorithm is in use.
 *
 * @param filename An absolute path to the file to hash.
 * @return A pointer to the hash or `NULL` on failure. It is the caller's
 *   responsibility to free the returned pointer.
 */
char *filehash(const char *filename);

/** \brief Copy a file, preserving the permissions, owner and group if
 * possible.
 *
 * @param from Absolute path of source.
 * @param to Absolute path of destination.
 * @return 0 on success, -1 on failure.
 */
int cp(const char *from, const char *to);

/** \brief Equivalent of `mkdir -p`.
 *
 * @param path An absolute or relative path to the final directory to create.
 * @return 0 on success, -1 on failure.
 */
int mkdirp(const char *path);

/** \brief Equivalent of `du -cS`.
 * 
 * @param path An absolute or relative path to a directory whose contents to
 *   measure. Note that subdirectories and special files are not taken into
 *   account.
 * @return -1 on failure or the total size of files in the directory on
 *   success.
 */
ssize_t du(const char *path);

/** \brief Remove files within a directory up to a certain limit.
 *
 * @param path An absolute or relative path to the directory to prune entries
 *   from.
 * @param reduction Remove files until we have removed at least this many
 *   bytes.
 * @return The number of bytes that were removed.
 */
size_t reduce(const char *path, size_t reduction);

/** \brief fgets-/getdelim-alike.
 *
 * It's possible I'm too tired or dense at this point, but all the POSIX
 * functions I'm aware of seem to have some deal-breaking flaw for reading a
 * '\0'-terminated string into a static buffer. Perhaps reading programmatic
 * strings from a file descriptor is not a common task.
 *
 * @param buffer Destination to read into.
 * @param limit Maximum bytes to read.
 * @param f Source file descriptor to read from.
 * @return true if data was read, false on EOF or no bytes read.
 */
bool get(char *buffer, size_t limit, FILE *f);

typedef struct file_iter file_iter_t;
file_iter_t *file_iter(const char *path);
void file_iter_destroy(file_iter_t *fi);
char *file_iter_next(file_iter_t *fi);

/** \brief Convenience wrapper around `realloc` that frees the first argument
 * if allocation fails.
 *
 * @param ptr Initial pointer.
 * @param size Number of bytes to allocate for the new area.
 * @return New pointer to use or `NULL` on failure.
 */
void *ralloc(void *ptr, size_t size);

/** \brief An alternative to readlink that NUL-terminates the output buffer.
 *
 * @param path Symlink to read target of.
 * @return Target path of the symlink.
 */
char *readln(const char *path);

/** \brief Retrieve a resolved path to ourselves.
 *
 * @return A fully resolved path to our own executable. The returned pointer
 *   should be freed by the caller.
 */
char *my_exe(void);

/** \brief Retrieve a resolved path to the first executable we shadow.
 *
 * Based on our environment variable $PATH, there are potentially several
 * executables that could be referred to by our argv[0]. By the fact that we
 * are running, we know that we are the first to be resolved. The following
 * function retrieves the second to be resolved. Note that this skips any
 * secondary (shadowed) executables that are just symlinks back to ourself.
 *
 * @param argv0 The value of argv[0] passed to `main`.
 * @return A resolved path to our first shadow. The returned pointer should be
 *   freed by the caller. `NULL` is returned if we're not shadowing anything.
 */
char *my_shadow(const char *argv0);

#endif
