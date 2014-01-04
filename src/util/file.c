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

static char *hex(unsigned char *hash) {
    char *h = (char*)malloc(MD5_DIGEST_LENGTH * 2 + 1);
    if (h == NULL)
        return NULL;

    char *p = h;
    for (unsigned int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(p, "%02x", hash[i]);
        p += 2;
    }

    return h;
}

char *filehash(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        return NULL;

    /* Measure the size of the file. */
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return NULL;
    }
    size_t sz = st.st_size;

    /* Mmap the file for MD5. If the file is empty then we avoid mmaping as it
     * will return failure and is not necessary.
     */
    void *addr = st.st_size == 0 ? NULL :
        mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    unsigned char *h = (unsigned char*)malloc(MD5_DIGEST_LENGTH);
    if (h == NULL) {
        if (addr != NULL)
            munmap(addr, sz);
        close(fd);
        return NULL;
    }
    MD5((unsigned char*)addr, sz, h);

    /* Success! */

    if (addr != NULL)
        munmap(addr, sz);
    close(fd);

    char *ph = hex(h);
    /* Note, it's possible hex just failed. In this case we naturally free h
     * and return NULL to the caller anyway.
     */
    free(h);
    return ph;
}

int mkdirp(const char *path) {
    assert(path != NULL);

    char *abspath;
    if (path[0] == '/')
        /* Path is already absolute. */
        abspath = (char*)path;
    else {
        char *cwd = getcwd(NULL, 0);
        if (cwd == NULL)
            return -1;
        abspath = (char*)realloc(cwd, strlen(cwd) + 1 + strlen(path) + 1);
        if (abspath == NULL) {
            free(cwd);
            return -1;
        }
        strcat(abspath, "/");
        strcat(abspath, path);
    }

    /* We commit a white lie, in that we told the caller we weren't going to
     * modify their string and we do. I think it's acceptable because we undo
     * the changes we make.
     */
    for (char *p = abspath + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            int r = mkdir(path, 0775);
            if (r != 0 && errno != EEXIST) {
                *p = '/';
                if (path[0] != '/')
                    free(abspath);
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(path, 0775) != 0 && errno != EEXIST) {
        if (path[0] != '/')
            free(abspath);
        return -1;
    }
    if (path[0] != '/')
        free(abspath);
    return 0;
}

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
