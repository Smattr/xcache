#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../util.h"

int cp(const char *from, const char *to) {
    assert(from != NULL);
    assert(to != NULL);

    /* Measure the size of the file. */
    struct stat st;
    if (stat(from, &st) != 0)
        return -1;
    size_t sz = st.st_size;

    /* We need to chmod the file in case it's not readable to us currently. */
    chmod(from, 0400);
    int in = open(from, O_RDONLY);
    if (in < 0) {
        chmod(from, st.st_mode);
        return -1;
    }

    /* Copy the file. */
    int out;
    if (!strcmp(to, "/dev/stdout")) {
        out = STDOUT_FILENO;
    } else if (!strcmp(to, "/dev/stderr")) {
        out = STDERR_FILENO;
    } else {
        out = open(to, O_WRONLY|O_CREAT, 0200);
        if (out < 0) {
            close(in);
            chmod(from, st.st_mode);
            return -1;
        }
    }
    ssize_t written = sendfile(out, in, NULL, sz);
    if (out != STDOUT_FILENO && out != STDERR_FILENO)
        close(out);
    close(in);
    chmod(from, st.st_mode);
    if ((size_t)written != sz) {
        /* We somehow failed to copy the entire file. */
        unlink(to);
        return -1;
    }
    return 0;
}
