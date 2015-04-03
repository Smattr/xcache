#include <dirent.h>
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

/* Turn a blob of hash data into readable hex output. */
static char *hex(unsigned char *hash) {
    char *h = malloc(MD5_DIGEST_LENGTH * 2 + 1);
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
    /* Measure the file. */
    struct stat st;
    if (stat(filename, &st) != 0) {
        /* Failed to stat. */
        return NULL;
    }

    bool permissions_altered = false;

    if (access(filename, R_OK) != 0) {
        /* We can't read the file. We'll need to twiddle its permission bits to
         * allow this and reset them afterwards. Note that this temporarily
         * exposes the file as readable by everyone.
         */
        mode_t mode = st.st_mode | S_IRUSR | S_IRGRP | S_IROTH;
        if (chmod(filename, mode) != 0) {
            /* Failed to make file readable. */
            return NULL;
        }
        permissions_altered = true;
    }

    char *ph = NULL; /* the final hash we'll return */

    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        goto end;

    size_t sz = st.st_size;

    /* Mmap the file for MD5. If the file is empty then we avoid mmaping as it
     * will return failure and is not necessary.
     */
    void *addr = st.st_size == 0 ? NULL :
        mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        goto end;
    }

    unsigned char *h = malloc(MD5_DIGEST_LENGTH);
    if (h == NULL) {
        if (addr != NULL)
            munmap(addr, sz);
        close(fd);
        goto end;
    }
    MD5((unsigned char*)addr, sz, h);

    /* Success! */

    if (addr != NULL)
        munmap(addr, sz);
    close(fd);

    ph = hex(h);
    /* Note, it's possible hex just failed. In this case we naturally free h
     * and return NULL to the caller anyway.
     */
    free(h);

end:
    if (permissions_altered) {
        /* Reset the original permissions. Ignore failure because there's not
         * much we can do about it.
         */
        (void)chmod(filename, st.st_mode);
    }

    return ph;
}
