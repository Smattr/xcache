#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../util.h"

size_t reduce(const char *path, size_t reduction) {
    assert(path != NULL);
    assert(reduction > 0);

    file_iter_t *fi = file_iter(path);
    if (fi == NULL)
        return 0;

    size_t removed = 0;
    char *fname;
    while (removed < reduction && (fname = file_iter_next(fi)) != NULL) {
        struct stat st;
        if (stat(fname, &st) != 0) {
            file_iter_destroy(fi);
            return removed;
        }
        if (unlink(fname) != 0) {
            file_iter_destroy(fi);
            return removed;
        }
        removed += st.st_size;
    }

    if (fname != NULL)
        file_iter_destroy(fi);

    return removed;
}
