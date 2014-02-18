#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../util.h"

static const char *atom(char **path) {
    char *start = *path;
    while (*start == '/')
        start++;
    if (*start == '\0')
        return NULL;

    char *end = start;
    while (*end != '\0' && *end != '/')
        end++;
    if (*end == '\0') {
        *path = end;
    } else {
        *path = end + 1;
        *end = '\0';
    }

    return start;
}

static char *append(char *dest, char *src) {
    assert(dest != NULL);

    size_t len = strlen(dest);
    size_t sz = len;

    const char *a;
    while ((a = atom(&src)) != NULL) {
        if (!strcmp(a, ".")) {
            continue;
        } else if (!strcmp(a, "..")) {
            if (!strcmp(dest, ""))
                continue;
            len--;
            while (dest[len] != '/')
                len--;
            dest[len] = '\0';
        } else {
            size_t alen = strlen(a);
            if (len + 1 + alen > sz) {
                char *tmp = realloc(dest, len + alen + 2);
                if (tmp == NULL) {
                    free(dest);
                    return NULL;
                }
                dest = tmp;
                sz = len + alen + 1;
            }
            sprintf(dest + len, "/%s", a);
            len += alen + 1;
        }
    }
    return dest;
}

char *abspath(char *relpath) {
    assert(relpath != NULL);

    char *abs = strdup("");
    if (abs == NULL)
        return NULL;

    if (relpath[0] != '/') {
        char *cwd = getcwd(NULL, 0);
        if (cwd == NULL) {
            free(abs);
            return NULL;
        }
        abs = append(abs, cwd);
        free(cwd);
        if (abs == NULL)
            return NULL;
    }

    abs = append(abs, relpath);
    return abs;
}
