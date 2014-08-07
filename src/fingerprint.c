#include "fingerprint.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

fingerprint_t *fingerprint(unsigned int argc, const char **argv) {
    fingerprint_t *f = calloc(1, sizeof(*f));
    if (f == NULL)
        goto fail;

    f->cwd = strdup(getcwd(NULL, 0));
    if (f->cwd == NULL)
        goto fail;

    f->arg_lens = calloc(argc, sizeof(*f->arg_lens));
    if (f->arg_lens == NULL)
        goto fail;
    f->arg_lens_sz = argc;

    unsigned int len = 0;
    for (unsigned int i = 0; i < argc; i++) {
        f->arg_lens[i] = strlen(argv[i]);
        if (i != 0)
            len++;
        len += f->arg_lens[i];
    }

    f->argv = malloc(sizeof(char) * (len + 1));
    if (f->argv == NULL)
        goto fail;

    char *p = f->argv;
    for (unsigned int i = 0; i < argc; i++) {
        if (i > 0)
            *p++ = ' ';
        strncpy(p, argv[i], f->arg_lens[i]);
        p += f->arg_lens[i];
    }
    *p = '\0';

    return f;

fail:
    fingerprint_destroy(f);
    return NULL;
}

void fingerprint_destroy(fingerprint_t *fp) {
    if (fp != NULL) {
        if (fp->arg_lens != NULL)
            free(fp->arg_lens);
        if (fp->cwd != NULL)
            free(fp->cwd);
        free(fp);
    }
}
