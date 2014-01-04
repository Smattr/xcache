#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include "file.h"
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

ssize_t reduce(const char *path, ssize_t reduction) {
    assert(path != NULL);
    assert(reduction > 0);

    file_iter_t *fi = file_iter(path);
    if (fi == NULL)
        return -1;

    ssize_t removed = 0;
    char *fname;
    while (removed < reduction && (fname = file_iter_next(fi)) != NULL) {
        struct stat st;
        if (stat(fname, &st) != 0) {
            file_iter_destroy(fi);
            return -1;
        }
        if (unlink(fname) != 0) {
            file_iter_destroy(fi);
            return -1;
        }
        removed += st.st_size;
    }

    if (fname != NULL)
        file_iter_destroy(fi);

    return removed;
}
