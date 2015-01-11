#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "../test.h"
#include <unistd.h>
#include "../util.h"

/* Returns a path component from the front of the given string and updates the
 * pointer to point at the remaining path. Returns NULL on no more path
 * elements remaining. As the input pointer is modified, the caller should
 * retain a copy of the pointer if they need to free this memory later.
 */
STATIC const char *atom(char **path) {
    char *start = *path;
    while (*start == '/')
        start++;
    if (*start == '\0')
        return NULL;

    char *end = start;
    while (*end != '\0' && *end != '/')
        end++;
    if (*end == '\0') {
        *path = end;
    } else {
        *path = end + 1;
        *end = '\0';
    }

    return start;
}

/* Add a path atom to an existing prefix. 'dest' is assumed not to end in / and
 * have length 'dest_len'. The length of 'dest' is only passed as an argument
 * to avoid repeatedly calculating it when calling this function in a loop.
 * 'src' must be a path atom. 'dest' is assumed to point to available memory of
 * at least PATH_MAX bytes. Returns the resulting size of 'dest' after appending
 * the atom.
 */
STATIC size_t append(char *dest, size_t dest_len, const char *src) {
    assert(dest != NULL);
    assert(strlen(dest) == dest_len);
    assert(src != NULL);
    assert(strcmp(src, "") && "append of invalid atom");
    assert(strchr(src, '/') == NULL && "invalid append of non-atom");

    if (!strcmp(src, ".")) {
        /* Nothing need be done. */
        
    } else if (!strcmp(src, "..")) {
        /* Strip a path atom or do nothing if dest == "". */
        while (dest_len > 0 && dest[dest_len] != '/')
            dest_len--;
        dest[dest_len] = '\0';

    } else {
        assert(dest_len + 2 + strlen(src) <= PATH_MAX);
        sprintf(dest + dest_len, "/%s", src);
        dest_len += strlen(src) + 1;
    }

    return dest_len;
}

void normpath(char *dest, char *src) {
    assert(dest != NULL);
    assert(src != NULL);

    size_t len = strlen(dest);

    const char *a;
    while ((a = atom(&src)) != NULL)
        len = append(dest, len, a);
}

char *abspath(char *relpath) {
    assert(relpath != NULL);

    char *abs = malloc(sizeof(char) * PATH_MAX);
    if (abs == NULL)
        return NULL;

    if (relpath[0] == '/') {
        /* This path is absolute. */
        abs[0] = '\0';
    } else {
        char *cwd = getcwd(abs, PATH_MAX);
        if (cwd == NULL) {
            free(abs);
            return NULL;
        }
    }

    normpath(abs, relpath);

    return abs;
}
