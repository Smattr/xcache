#include <assert.h>
#include "cache.h"
#include "depset.h"
#include <fcntl.h>
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
#include "util.h"

static bool dryrun = false;

static const char *cache_dir = NULL;

static bool hook_getenv = true;

static void usage(const char *prog) {
    fprintf(stderr, "Usage:\n"
        "  %s [options] command args...\n"
        "\n"
        "Options:\n"
        "  --cache-dir <dir>\n"
        "  -c <dir>           Locate cache in <dir>.\n"
        "  --dry-run\n"
        "  -n                 Simulate only; do not perform any actions.\n"
        "  --no-getenv\n"
        "  -e                 Do not hook getenv.\n"
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
    return aprintf("%s/.xcache", home);
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
        } else if (!strcmp(argv[index], "--no-getenv") ||
                   !strcmp(argv[index], "-e")) {
            hook_getenv = false;
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
            printf("xcache %d.%02d\n", VERSION_MAJOR, VERSION_MINOR);
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


/* Sanity checks on open flags because checking for O_RDONLY is awkward. */
typedef char FILE_FLAGS_AS_EXPECTED[
    O_RDONLY == 00 && O_WRONLY == 01 && O_RDWR == 02 ? 1 : -1];
/* See usage of this below. */
static const int FLAG_MASK = O_RDONLY | O_WRONLY | O_RDWR;

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

    cache_t *cache = cache_open(cache_dir);
    if (cache == NULL) {
        ERROR("Failed to create cache\n");
        return -1;
    }

    int id = cache_locate(cache, argc - index, &argv[index]);
    if (id >= 0) {
        /* Excellent news! We found a cache entry and don't need to run the
         * target program.
         */
        DEBUG("Found matching cache entry\n");
        int res = cache_dump(cache, id);
        cache_close(cache);
        return res;
    }

    /* If we've reached this point, we failed to locate a suitable cached entry
     * for this execution. We need to actually run the program itself.
     */

    depset_t *deps = depset_new();
    if (deps == NULL) {
        ERROR("Failed to create dependency set\n");
        return -1;
    }

    tracee_t *target = trace(&argv[index], hook_getenv ? argv[0] : NULL);
    if (target == NULL) {
        ERROR("Failed to start and trace target %s\n", argv[index]);
        return -1;
    }

    bool success = false;

    syscall_t *s;
    while ((s = next_syscall(target)) != NULL) {

#define ADD_AS(category, argno) \
    do { \
        char *_f = syscall_getstring(s, (argno)); \
        if (_f == NULL) { \
            if (s->call == SYS_execve) { \
                /* A successful execve results in two entry SIGTRAPs, the
                 * second one with an argument of NULL. Presumably the second
                 * trap is an artefact of the program loader.
                 */ \
                break; \
            } \
            DEBUG("Failed to retrieve string argument %d from syscall %s " \
                "(%ld)\n", (argno), translate_syscall(s->call), s->call); \
            goto bailout; \
        } \
        char *_fabs = abspath(_f); \
        if (_fabs == NULL) { \
            DEBUG("Failed to resolve path \"%s\"\n", _f); \
            goto bailout; \
        } \
        int _r = depset_add_##category(deps, _fabs); \
        if (_r != 0) { \
            DEBUG("Failed to add " #category " \"%s\"\n", _fabs); \
            free(_fabs); \
            goto bailout; \
        } \
        free(_fabs); \
    } while (0)

        /* Any syscall we receive may be the kernel entry or exit. Handle entry
         * separately first because there are relatively few syscalls where
         * entry is relevant for us. The relevant ones are essentially ones
         * that destroy some resource we need to measure before it disappears.
         */
        if (s->enter) {
            switch (s->call) {

                /* We need to handle execve on kernel entry because our
                 * original address space containing the input argument is gone
                 * on kernel exit.
                 */
                case SYS_execve:
                    ADD_AS(input, 1);
                    break;

                case SYS_open:;
                    /* In the case where a file is being opened RW, we need to
                     * do our measurement beforehand in case the user is using
                     * a flag like O_CREAT that makes measurement ambiguous
                     * when done afterwards. To simplify things, we handle RO
                     * open here as well.
                     */
                    int flags = (int)syscall_getarg(s, 2);
                    int mode = flags & FLAG_MASK;

                    /* If we're opening this file write-only, we don't need to
                     * do any measurement before opening as this file is purely
                     * an output.
                     */
                    if (mode == O_WRONLY)
                        break;

                    /* If we're opening this file read-write, there are some
                     * extra conditions that may lead us to bail out.
                     */
                    if (mode == O_RDWR) {

                        /* If a file is opened with O_CREAT and O_EXCL, the
                         * open fails if the file exists. In other words, even
                         * if we are opening this file O_RDWR, we are treating
                         * it as only an output.
                         */
                        if ((flags & O_CREAT) && (flags & O_EXCL))
                            break;

                        /* If a file is opened with O_TRUNC, we're ignoring its
                         * current contents and hence treating it purely as an
                         * output. O_TRUNC actually has no effect if the file is
                         * a device or a fifo, but regardless the caller is
                         * clearly not expecting to depend on the existing
                         * contents.
                         */
                        if (flags & O_TRUNC)
                            break;

                    }
                    ADD_AS(input, 1);
                    break;

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
                        translate_syscall(s->call), s->call);
                    goto bailout;

                default:
                    IDEBUG("irrelevant syscall entry %s (%ld)\n",
                        translate_syscall(s->call), s->call);
            }
            acknowledge_syscall(s);
            continue;
        }

        /* We should now only be handling syscall exits. */
        assert(!s->enter);

        switch (s->call) {

            case SYS_access:
                ADD_AS(input, 1);
                break;

            case SYS_chmod:
                ADD_AS(output, 1);
                break;

            case SYS_creat:
                ADD_AS(output, 1);
                break;

            case SYS_open:;
                /* Note that we are only handling the 'write' aspects of an
                 * open call here, because the 'read' aspects were handled on
                 * syscall entry.
                 */
                int flags = (int)syscall_getarg(s, 2);
                int mode = flags & FLAG_MASK;
                if (mode == O_WRONLY || mode == O_RDWR)
                    ADD_AS(output, 1);
                break;

            case SYS_readlink:
                ADD_AS(input, 1);
                break;

            case SYS_stat:
                ADD_AS(input, 1);
                break;

            case SYS__sysctl:
            case SYS_acct:
            case SYS_chdir:
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
            case SYS_readlinkat:
            case SYS_rename:
            case SYS_renameat:
            case SYS_statfs:
            case SYS_swapoff:
            case SYS_swapon:
            case SYS_symlink:
            case SYS_symlinkat:
            case SYS_truncate:
#if __WORDSIZE == 32
            /* umount is not available on a 64-bit kernel. */
            case SYS_umount:
#endif
            case SYS_umount2:
            case SYS_uselib:
                DEBUG("bailing out due to unhandled syscall %s (%ld)\n",
                    translate_syscall(s->call), s->call);
                goto bailout;

            default:
                IDEBUG("irrelevant syscall exit %s (%ld)\n",
                    translate_syscall(s->call), s->call);
        }
        acknowledge_syscall(s);

#undef ADD_AS

    }

    /* If we've reached here (i.e. not jumped to 'bailout'), we successfully
     * traced the target. Hence we can now cache its dependency set for
     * retrieval on a later run.
     */
    success = true;

bailout:;
    int ret = complete(target);

    const char *outfile = get_stdout(target),
               *errfile = get_stderr(target);

    if (success && ret == 0) {
        DEBUG("Adding cache entry\n");
        if (cache_write(cache, argc - index, &argv[index], deps, outfile, errfile) != 0)
            /* This failure is non-critical in a sense. */
            DEBUG("Failed to write entry to cache\n");
    }

    if (outfile != NULL)
        unlink(outfile);
    if (errfile != NULL)
        unlink(errfile);

    cache_close(cache);
    delete(target);
    return ret;
}
