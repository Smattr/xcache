#include <fcntl.h>
#include "file.h"
#include <openssl/md5.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
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
