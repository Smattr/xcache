#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include "file.h"
#include "log.h"
#include <openssl/md5.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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


    /* Mmap the file for MD5. */
    void *addr = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    unsigned char *h = (unsigned char*)malloc(MD5_DIGEST_LENGTH);
    if (h == NULL) {
        munmap(addr, sz);
        close(fd);
        return NULL;
    }
    MD5((unsigned char*)addr, sz, h);

    /* Success! */

    munmap(addr, sz);
    close(fd);
    return (char*)h;
}

int cp(const char *from, const char *to) {
    assert(from != NULL);
    assert(to != NULL);

    int in = open(from, O_RDONLY);
    if (in < 0)
        return -1;

    /* Measure the size of the file. */
    struct stat st;
    if (fstat(in, &st) != 0) {
        close(in);
        return -1;
    }
    size_t sz = st.st_size;

    /* Copy the file. */
    int out = open(to, O_WRONLY|O_CREAT, st.st_mode);
    if (out < 0) {
        close(in);
        return -1;
    }
    ssize_t written = sendfile(out, in, NULL, sz);
    fchown(out, st.st_uid, st.st_gid);
    close(out);
    close(in);
    if ((size_t)written != sz) {
        /* We somehow failed to copy the entire file. */
        unlink(to);
        return -1;
    }
    return 0;
}

int mkdirp(const char *path) {
    assert(path != NULL);

    /* We commit a white lie, in that we told the caller we weren't going to
     * modify their string and we do. I think it's acceptable because we undo
     * the changes we make.
     */
    for (char *p = (char*)(path + 1); *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            int r = mkdir(path, 0775);
            if (r != 0 && errno != EEXIST) {
                DEBUG("Failed to create directory \"%s\"\n", path);
                *p = '/';
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(path, 0775) != 0 && errno != EEXIST) {
        DEBUG("Failed to create directory \"%s\"\n", path);
        return -1;
    }
    return 0;
}
