#include <assert.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../util.h"

char *my_exe(void) {
    return readln("/proc/self/exe");
}

/** \brief Is the given file a symlink?
 *
 * @param path A target path to check.
 * @return true if the path is a symlink, false otherwise.
 */
static bool is_symlink(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0)
        return false;
    return S_ISLNK(st.st_mode);
}

/** \brief Is the given file runnable (readable and executable)?
 *
 * @param path A target path to check.
 * @return true if the path is readable and executable, false otherwise.
 */
static bool is_executable(const char *path) {
    return access(path, R_OK|X_OK) == 0;
}

char *my_shadow(const char *argv0) {
    if (strchr(argv0, '/') == NULL) {
        /* We're not shadowing anything because we're not relying on $PATH. */
        return NULL;
    }

    const char *path = getenv("PATH");
    if (path == NULL)
        return NULL;

    autofree char *me = my_exe();
    if (me == NULL)
        return NULL; /* bummer */

    char candidate[PATH_MAX];

    const char *end = path;
    const char *start = path;
    while (true) {
loop:
        start = end;
        if (start == NULL)
            break;
        end = strchr(start, ':');

        int written;
        if (end == NULL) {
            /* Last element of $PATH */
            written = snprintf(candidate, sizeof(candidate), "%s/%s", start,
                argv0);
        } else {
            assert(end - start >= 0 && end - start < INT_MAX);
            written = snprintf(candidate, sizeof(candidate), "%.*s/%s",
                (int)(end - start), start, argv0);
        }

        if (written < 0 || written >= (int)sizeof(candidate)) {
            /* Overflowed `candidate` */
            continue;
        }

        if (!is_executable(candidate))
            continue;

        /* Resolve all symlinks */
        while (is_symlink(candidate)) {
            ssize_t sz = readlink(candidate, candidate, sizeof(candidate));
            if (sz <= 0 || sz >= (ssize_t)sizeof(candidate)) {
                /* `readlink` failed */
                goto loop; /* continue outer loop */
            }
            candidate[sz] = '\0';
        }

        if (strcmp(candidate, me) == 0) {
            /* We found ourselves! */
            continue;
        }

        /* If we reached here, we found an executable under the same name as
         * `argv0` that we are shadowing.
         */
        return strdup(candidate);
    }

    /* We exhausted $PATH and didn't find anything we are shadowing. */
    return NULL;
}
