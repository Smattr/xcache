#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *abspath(const char *relpath) {
    assert(relpath != NULL);
    if (relpath[0] == '/') {
        /* This path is already absolute. */
        return strdup(relpath);
    }
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL)
        return NULL;
    char *path = realloc(cwd, strlen(cwd) + 1 + strlen(relpath) + 1);
    if (path == NULL) {
        free(cwd);
        return NULL;
    }
    strcat(path, "/");
    strcat(path, relpath);
    return path;
}
