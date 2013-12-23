#include <assert.h>
#include "cache.h"
#include "config.h"
#include "depset.h"
#include <fcntl.h>
#include "file.h"
#include "log.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "trace.h"
#include "translate-syscall.h"
#include <unistd.h>

static bool dryrun = false;

static const char *cache_dir = NULL;

static void usage(const char *prog) {
    fprintf(stderr, "Usage:\n"
        "  %s [options] command args...\n"
        "\n"
        "Options:\n"
        "  --cache-dir <dir>\n"
        "  -c <dir>           Locate cache in <dir>.\n"
        "  --dry-run\n"
        "  -n                 Simulate only; do not perform any actions.\n"
        "  --help\n"
        "  -?                 Print this help information and exit.\n"
        "  --log <file>\n"
        "  -l <file>          Direct any output to <file>. Defaults to stderr.\n"
        "  --quiet\n"
        "  -q                 Show less output.\n"
        "  --verbose\n"
        "  -v                 Show more output.\n"
        "  --version          Output version information and then exit.\n"
        , prog);
}

/* Returns the default cache root, ${HOME}/.xcache. It is the caller's
 * responsibility to free the returned pointer.
 */
static char *default_cache_dir(void) {
    char *home = getenv("HOME");
    if (home == NULL)
        return NULL;
    char *d = (char*)malloc(strlen(home) + strlen("/.xcache") + 1);
    if (d == NULL)
        return NULL;
    sprintf(d, "%s/.xcache", home);
    return d;
}

/* Parse command-line arguments. Unfortunately getopt has some undesirable
 * behaviour that prevents us using it.
 *
 * Returns the index of the first non-xcache argument.
 */
static int parse_arguments(int argc, const char **argv) {
    int index;
    for (index = 1; index < argc; index++) {
        if ((!strcmp(argv[index], "--cache-dir") ||
             !strcmp(argv[index], "-c")) &&
            index < argc - 1) {
            cache_dir = argv[++index];
        } else if (!strcmp(argv[index], "--dry-run") ||
                   !strcmp(argv[index], "-n")) {
            dryrun = true;
        } else if ((!strcmp(argv[index], "--log") ||
                    !strcmp(argv[index], "-l")) &&
                   index < argc - 1) {
            if (log_init(argv[++index]) != 0) {
                usage(argv[0]);
                exit(-1);
            }
        } else if (!strcmp(argv[index], "--quiet") ||
                   !strcmp(argv[index], "-q")) {
            verbosity--;
        } else if (!strcmp(argv[index], "--verbose") ||
                   !strcmp(argv[index], "-v")) {
            verbosity++;
        } else if (!strcmp(argv[index], "--version")) {
            printf("%s %d.%02d\n", PROJ_NAME, VERSION_MAJOR, VERSION_MINOR);
            exit(0);
        } else if (!strcmp(argv[index], "--help") ||
                   !strcmp(argv[index], "-?")) {
            usage(argv[0]);
            exit(0);
        } else {
            break;
        }
    }
    return index;
}

int main(int argc, const char **argv) {
    int index = parse_arguments(argc, argv);

    if (argc - index == 0) {
        ERROR("No target command supplied\n");
        usage(argv[0]);
        return -1;
    }

    if (cache_dir == NULL) {
        cache_dir = default_cache_dir();
        if (cache_dir == NULL) {
            ERROR("Failed to determine default cache directory\n");
            return -1;
        }
    }

    if (mkdirp(cache_dir) != 0) {
        ERROR("Failed to create cache directory \"%s\"\n", cache_dir);
        return -1;
    }

    cache_t *cache = cache_open(cache_dir, DATA_SIZE_UNSET);
    if (cache == NULL) {
        ERROR("Failed to create cache\n");
        return -1;
    }

    int id = cache_locate(cache, &argv[index]);
    if (id >= 0) {
        /* Excellent news! We found a cache entry and don't need to run the
         * target program.
         */
        int res = cache_dump(cache, id);
        cache_close(cache);
        return res;
    }

    /* If we've reached this point, we failed to locate a suitable cached entry
     * for this execution. We need to actually run the program itself.
     */

    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        ERROR("Failed to read current working directory\n");
        cache_close(cache);
        return -1;
    }

    depset_t *deps = depset_new();
    if (deps == NULL) {
        ERROR("Failed to create dependency set\n");
        return -1;
    }

    proc_t *target = trace(&argv[index]);
    if (target == NULL) {
        ERROR("Failed to start and trace target %s\n", argv[index]);
        return -1;
    }

    syscall_t s;
    while (next_syscall(target, &s) == 0) {

#define ADD_AS(category, argno) \
    do { \
        char *_f = syscall_getstring(target, (argno)); \
        if (_f == NULL) { \
            DEBUG("Failed to retrieve string argument %d from syscall %s " \
                "(%ld)\n", (argno), translate_syscall(s.call), s.call); \
            goto bailout; \
        } \
        int _r = depset_add_##category(deps, _f); \
        if (_r != 0) { \
            DEBUG("Failed to add " #category " \"%s\"\n", _f); \
            goto bailout; \
        } \
    } while (0)

        /* Any syscall we receive may be the kernel entry or exit. Handle entry
         * separately first because there are relatively few syscalls where
         * entry is relevant for us. The relevant ones are essentially ones
         * that destroy some resource we need to measure before it disappears.
         */
        if (s.enter) {
            switch (s.call) {

                case SYS_rename:
                    ADD_AS(input, 1);
                    break;

                case SYS_unlink:
                    ADD_AS(input, 1);
                    break;

                case SYS_rmdir:
                    ADD_AS(input, 1);
                    break;

                case SYS_renameat:
                case SYS_unlinkat:
                    DEBUG("bailing out due to unhandled syscall %s (%ld)\n",
                        translate_syscall(s.call), s.call);
                    goto bailout;

#if 0
                default:
                    DEBUG("irrelevant syscall entry %s (%ld)\n",
                        translate_syscall(s.call), s.call);
#endif
            }
            acknowledge_syscall(target);
            continue;
        }

        /* We should now only be handling syscall exits. */
        assert(!s.enter);

        switch (s.call) {

            case SYS_access:
                ADD_AS(input, 1);
                break;

            case SYS_creat:
                ADD_AS(output, 1);
                break;

            case SYS_open: {
                int flags = (int)syscall_getarg(target, 2);
                char *fname = syscall_getstring(target, 1);
                int r = 0;
                if (fname == NULL) {
                    DEBUG("Failed to retrieve string argument 1 from " \
                        "syscall open (%ld)\n", (long)SYS_open);
                    goto bailout;
                }
                if (flags & O_WRONLY)
                    r |= depset_add_output(deps, fname);
                else {
                    r |= depset_add_input(deps, fname);
                    if (flags & O_RDWR)
                        r |= depset_add_output(deps, fname);
                }
                if (r != 0) {
                    DEBUG("Failed to add dependency \"%s\"\n", fname);
                    goto bailout;
                }
                break;
            }

            case SYS_stat:
                ADD_AS(input, 1);
                break;

            case SYS__sysctl:
            case SYS_acct:
            case SYS_chdir:
            case SYS_chmod:
            case SYS_chown:
            case SYS_chroot:
            case SYS_fchdir:
            case SYS_link:
            case SYS_linkat:
            case SYS_mkdir:
            case SYS_mkdirat:
            case SYS_mknod:
            case SYS_mknodat:
            case SYS_mount:
            case SYS_openat:
            case SYS_pivot_root:
            case SYS_readlink:
            case SYS_readlinkat:
            case SYS_rename:
            case SYS_renameat:
            case SYS_rmdir:
            case SYS_statfs:
            case SYS_swapoff:
            case SYS_swapon:
            case SYS_symlink:
            case SYS_symlinkat:
            case SYS_truncate:
            case SYS_unlink:
            case SYS_unlinkat:
#if __WORDSIZE == 32
            /* umount is not available on a 64-bit kernel. */
            case SYS_umount:
#endif
            case SYS_umount2:
            case SYS_uselib:
                DEBUG("bailing out due to unhandled syscall %s (%ld)\n",
                    translate_syscall(s.call), s.call);
                goto bailout;

#if 0
            default:
                DEBUG("irrelevant syscall exit %s (%ld)\n",
                    translate_syscall(s.call), s.call);
#endif
        }
        acknowledge_syscall(target);

#undef ADD_AS

    }

    /* If we've reached here (i.e. not jumped to 'bailout'), we successfully
     * traced the target. Hence we can now cache its dependency set for
     * retrieval on a later run.
     */
    if (cache_write(cache, cwd, &argv[index], deps) != 0)
        /* This failure is non-critical in a sense. */
        DEBUG("Failed to write entry to cache\n");

    int ret;
bailout:

    cache_close(cache);
    ret = complete(target);
    delete(target);
    return ret;
}
