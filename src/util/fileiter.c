#include <assert.h>
#include <dirent.h>
#include <errno.h>
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

struct file_iter {
    DIR *dir;
    off_t root_len;
    char *path;
};

file_iter_t *file_iter(const char *path) {
    assert(path != NULL);

    file_iter_t *fi = malloc(sizeof(*fi));
    if (fi == NULL)
        return NULL;

    fi->dir = opendir(path);
    if (fi->dir == NULL) {
        free(fi);
        return NULL;
    }

    fi->root_len = strlen(path) + 1;
    fi->path = malloc(fi->root_len + NAME_MAX + 1);
    if (fi->path == NULL) {
        closedir(fi->dir);
        free(fi);
        return NULL;
    }
    sprintf(fi->path, "%s/", path);
    return fi;
}

void file_iter_destroy(file_iter_t *fi) {
    free(fi->path);
    closedir(fi->dir);
    free(fi);
}

char *file_iter_next(file_iter_t *fi) {
    assert(fi != NULL);

    assert(fi->dir != NULL);
    errno = 0;
    struct dirent *ent;
    do {
        ent = readdir(fi->dir);
        if (ent == NULL) {
            int err = errno;
            file_iter_destroy(fi);
            errno = err;
            return NULL;
        }
    } while (ent->d_type != DT_REG);
    strcpy(fi->path + fi->root_len, ent->d_name);
    return fi->path;
}
