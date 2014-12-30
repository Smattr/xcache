/* The implementation below is based on basic, established RPC schemes. Data is
 * serialised with its length preceding it.
 * There are a couple of main advantages to this approach:
 *  1. We can encode the NULL pointer as length 0; and
 *  2. We can determine how much memory is required for data before reading it.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ssize_t read_data(int fd, unsigned char **data) {
    assert(data != NULL);

    size_t len;
    ssize_t sz = read(fd, &len, sizeof(len));
    if (sz == 0)
        return -2;
    if (sz != sizeof(len))
        return -1;

    if (len == 0) {
        *data = NULL;
        return 0;
    }

    *data = malloc(sizeof(*data[0]) * len);
    if (*data == NULL)
        return -1;

    sz = read(fd, *data, len);
    if (sz < 0 || (size_t)sz != len) {
        free(*data);
        return -1;
    }

    return len;
}

int write_data(int fd, const unsigned char *data, size_t len) {
    /* Send the size of the data. */
    ssize_t sz = write(fd, &len, sizeof(len));
    if (sz != sizeof(len))
        return -1;

    /* Send the data itself. */
    if (len > 0) {
        sz = write(fd, data, len);
        if (sz < 0 || (size_t)sz != len)
            return -1;
    }

    return 0;
}
