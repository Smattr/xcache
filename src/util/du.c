#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <stdbool.h>
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

ssize_t du(const char *path) {
    assert(path != NULL);

    file_iter_t *fi = file_iter(path);
    if (fi == NULL)
        return -1;

    ssize_t sz = 0;
    char *fname;
    while ((fname = file_iter_next(fi)) != NULL) {
        struct stat st;
        if (stat(fname, &st) != 0) {
            file_iter_destroy(fi);
            return -1;
        }
        sz += st.st_size;
    }
    return sz;
}
