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
