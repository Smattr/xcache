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
