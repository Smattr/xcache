/* The implementation below is based on basic, established RPC mechanisms.
 * Strings are serialised with their length (including trailing \0) preceding
 * them. Their are a couple of main advantages to this approach:
 *  1. We can encode the NULL pointer as length 0; and
 *  2. We can determine how much memory is required for a string before reading
 *     it.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int read_string(int fd, char **data) {
    assert(data != NULL);

    size_t len;
    ssize_t sz = read(fd, &len, sizeof(len));
    if (sz == 0)
        return 1;
    if (sz != sizeof(len))
        return -1;

    if (len == 0) {
        /* NULL marker. */
        *data = NULL;
        return 0;
    }

    *data = malloc(sizeof(char) * len);
    if (*data == NULL)
        return -1;

    sz = read(fd, *data, len - 1);
    if (sz < 0 || (size_t)sz != len - 1) {
        free(*data);
        return -1;
    }

    (*data)[len - 1] = '\0';
    return 0;
}

int write_string(int fd, const char *data) {
    if (data == NULL) {
        /* Send a size of 0 as a NULL marker. */
        size_t marker = 0;
        ssize_t sz = write(fd, &marker, sizeof(marker));
        if (sz != sizeof(marker))
            return -1;
        return 0;
    }

    /* Send the size of the string, including \0. */
    size_t len = (int)strlen(data) + 1;
    ssize_t sz = write(fd, &len, sizeof(len));
    if (sz != sizeof(len))
        return -1;

    /* Send the data itself. */
    sz = write(fd, data, len - 1);
    if (sz < 0 || (size_t)sz != len - 1)
        return -1;

    return 0;
}
