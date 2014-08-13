#ifndef _XCACHE_ENVIRON_
#define _XCACHE_ENVIRON_

/* Copy our existing environment and extend it to LD_PRELOAD the given library.
 * Returns NULL on failure.
 */
char **env_ld_preload(const char *lib);

#endif
