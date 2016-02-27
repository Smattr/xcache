#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../util.h"

int mkdirp(const char *path) {
    assert(path != NULL);

    char *abspath;
    if (path[0] == '/') {
        /* Path is already absolute. */
        abspath = strdup(path);
        if (abspath == NULL)
            return -1;
    } else {
        char *cwd = getcwd(NULL, 0);
        if (cwd == NULL)
            return -1;
        abspath = ralloc(cwd, strlen(cwd) + 1 + strlen(path) + 1);
        if (abspath == NULL)
            return -1;
        strcat(abspath, "/");
        strcat(abspath, path);
    }

    for (char *p = abspath + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            int r = mkdir(path, 0775);
            if (r != 0 && errno != EEXIST) {
                free(abspath);
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(path, 0775) != 0 && errno != EEXIST) {
        free(abspath);
        return -1;
    }
    free(abspath);
    return 0;
}
