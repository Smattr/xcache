#include <assert.h>
#include <dirent.h>
#include <errno.h>
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
