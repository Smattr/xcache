#include <openssl/md5.h>
#include <stddef.h>
#include <stdlib.h>

char *filehash(char *data, size_t len) {
    unsigned char *h = (unsigned char*)malloc(MD5_DIGEST_LENGTH);
    if (h == NULL) {
        return NULL;
    }

    MD5((unsigned char*)data, len, h);
    return (char*)h;
}
