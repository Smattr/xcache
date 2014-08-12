#define _GNU_SOURCE
#include <assert.h>
#include "environ.h"
#include <stdlib.h>

extern char **environ;

const char **env_new(const char **extra) {
    unsigned int len;
    for (len = 0; environ[len] != NULL; len++);
    unsigned int len2;
    for (len2 = 0; extra[len2] != NULL; len2++);
    len += len2;
    const char **new = malloc(sizeof(char*) * (len + 1));
    if (new == NULL)
        return NULL;
    unsigned int i;
    for (i = 0; environ[i] != NULL; i++)
        new[i] = environ[i];
    unsigned int j;
    for (j = 0; extra[j] != NULL; j++)
        new[i + j] = extra[j];
    assert(i + j == len);
    new[len] = NULL;
    return new;
}
