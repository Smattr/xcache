#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include "../util.h"

char *readln(const char *path) {
    char *resolved = malloc(sizeof(char) * PATH_MAX);
    if (resolved == NULL)
        return NULL;

    ssize_t len = readlink(path, resolved, PATH_MAX);
    if (len == -1 || len >= PATH_MAX) {
        free(resolved);
        return NULL;
    }

    resolved[len] = '\0';
    return resolved;
}
