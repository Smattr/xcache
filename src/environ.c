#define _GNU_SOURCE
#include <assert.h>
#include "environ.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

extern char **environ;

/* FIXME: We need to account for (or guard against) the issue that we can't
 * LD_PRELOAD a library from a path containing spaces.
 */

char **env_ld_preload(const char *lib) {

    /* Count size of existing environment. */
    unsigned int len;
    for (len = 0; environ[len] != NULL; len++);
    /* One more in case LD_PRELOAD is not yet set. */
    len++;

    char **new = malloc(sizeof(char*) * (len + 1));
    if (new == NULL)
        return NULL;

    /* Setup the new environment. */
    bool appended = false;
    for (unsigned int i = 0; environ[i] != NULL; i++) {
        if (strncmp(environ[i], "LD_PRELOAD=", strlen("LD_PRELOAD=")) == 0) {
            char *t = aprintf("LD_PRELOAD=%s %s", lib,
                environ[i][strlen("LD_PRELOAD=")]);
            if (t == NULL) {
                free(new);
                return NULL;
            }
            /* FIXME: we leak memory here. */
            new[i] = t;
            appended = true;
        } else {
            new[i] = environ[i];
        }
    }

    if (appended) {
        /* We've already set LD_PRELOAD. */
        new[len - 2] = NULL;
    } else {
        /* We need to append it now. */
        char *t = aprintf("LD_PRELOAD=%s", lib);
        if (t == NULL) {
            free(new);
            return NULL;
        }
        /* FIXME: Again, we leak memory here. */
        new[len - 2] = t;
        new[len - 1] = NULL;
    }

    return new;
}
