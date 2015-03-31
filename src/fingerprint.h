/* Functions for generating data that uniquely identifies an invocation of a
 * process with arguments. We want to be able to write the resulting
 * fingerprint into a database, so all members need to be of formats we can
 * store in SQLite. Note that the arg_lens field is stored as a BLOB.
 */

#ifndef _XCACHE_FINGERPRINT_
#define _XCACHE_FINGERPRINT_

#include <stdint.h>

typedef struct {
    /* Current working directory */
    char *cwd;

    /* Characters is each argument */
    unsigned int *arg_lens;
    unsigned int arg_lens_sz;

    /* Concatenated arguments */
    char *argv;
} fingerprint_t;

/* Create a fingerprint for the given invocation. Returns NULL on failure. */
fingerprint_t *fingerprint(unsigned int argc, char **argv);

/* Deallocate memory associated with a fingerprint. */
void fingerprint_destroy(fingerprint_t *fp);

#endif
