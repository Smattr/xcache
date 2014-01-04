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

typedef struct {
    DIR *dir;
    off_t root_len;
    char *path;
} file_iter_t;

static file_iter_t *get_iter(const char *path) {
    assert(path != NULL);

    file_iter_t *fi = (file_iter_t*)malloc(sizeof(*fi));
    if (fi == NULL)
        return NULL;

    fi->dir = opendir(path);
    if (fi->dir == NULL) {
        free(fi);
        return NULL;
    }

    fi->root_len = strlen(path) + 1;
    fi->path = (char*)malloc(fi->root_len + NAME_MAX + 1);
    if (fi->path == NULL) {
        closedir(fi->dir);
        free(fi);
        return NULL;
    }
    sprintf(fi->path, "%s/", path);
    return fi;
}

static void destroy_iter(file_iter_t *fi) {
    free(fi->path);
    closedir(fi->dir);
    free(fi);
}

static char *next(file_iter_t *fi) {
    assert(fi != NULL);

    assert(fi->dir != NULL);
    errno = 0;
    struct dirent *ent;
    do {
        ent = readdir(fi->dir);
        if (ent == NULL) {
            int err = errno;
            destroy_iter(fi);
            errno = err;
            return NULL;
        }
    } while (ent->d_type != DT_REG);
    strcpy(fi->path + fi->root_len, ent->d_name);
    return fi->path;
}


ssize_t du(const char *path) {
    assert(path != NULL);

    file_iter_t *fi = get_iter(path);
    if (fi == NULL)
        return -1;

    ssize_t sz = 0;
    char *fname;
    while ((fname = next(fi)) != NULL) {
        struct stat st;
        if (stat(fname, &st) != 0) {
            destroy_iter(fi);
            return -1;
        }
        sz += st.st_size;
    }
    return sz;
}

ssize_t reduce(const char *path, ssize_t reduction) {
    assert(path != NULL);
    assert(reduction > 0);

    file_iter_t *fi = get_iter(path);
    if (fi == NULL)
        return -1;

    ssize_t removed = 0;
    char *fname;
    while (removed < reduction && (fname = next(fi)) != NULL) {
        struct stat st;
        if (stat(fname, &st) != 0) {
            destroy_iter(fi);
            return -1;
        }
        if (unlink(fname) != 0) {
            destroy_iter(fi);
            return -1;
        }
        removed += st.st_size;
    }

    if (fname != NULL)
        destroy_iter(fi);

    return removed;
}

bool get(char *buffer, size_t limit, FILE *f) {
    if (limit == 0)
        return false;
    while (limit > 1) {
        int c = fgetc(f);
        if (c == EOF)
            return false;
        else if (c == '\0')
            break;
        *buffer++ = c;
        limit--;
    }
    assert(limit >= 1);
    *buffer = '\0';
    return true;
}
