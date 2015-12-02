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

static bool directories = false;

static bool statistics = true;

/* Paths to never consider as inputs. This is to avoid tracking things that are
 * not conceptually files, but rather Linux APIs. Entries to this array should
 * be path prefixes.
 */
static const struct {
    const char *prefix;
    size_t length;
} exclude[] = {
#define EXCLUDE_ENTRY(pref) { .prefix = pref, .length = sizeof(pref) - 1 }
    EXCLUDE_ENTRY("/dev/"),
    EXCLUDE_ENTRY("/proc/"),
#undef EXCLUDE_ENTRY
};
static const size_t exclude_sz = sizeof(exclude) / sizeof(exclude[0]);

static void usage(const char *prog) {
    fprintf(stderr, "Usage:\n"
        "  %s [options] command args...\n"
        "\n"
        "Options:\n"
        "  --cache-dir <dir>\n"
        "  -c <dir>           Locate cache in <dir>.\n"
        "  --directories\n"
        "  -d                 Track directories as well as files.\n"
        "  --dry-run\n"
        "  -n                 Simulate only; do not perform any actions.\n"
        "  --no-directories\n"
        "  -D                 Do not track directories; only files.\n"
        "  --no-getenv\n"
        "  -e                 Do not hook getenv.\n"
        "  --help\n"
        "  -?                 Print this help information and exit.\n"
        "  --log <file>\n"
        "  -l <file>          Direct any output to <file>. Defaults to stderr.\n"
        "  --no-statistics    Do not log statistics in cache database.\n"
        "  --quiet\n"
        "  -q                 Show less output.\n"
        "  --statistics       Log statistics in cache database (default).\n"
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
static int parse_arguments(int argc, char **argv) {
    int index;
    for (index = 1; index < argc; index++) {
        if ((!strcmp(argv[index], "--cache-dir") ||
             !strcmp(argv[index], "-c")) &&
            index < argc - 1) {
            cache_dir = argv[++index];
        } else if (!strcmp(argv[index], "--directories") ||
                   !strcmp(argv[index], "-d")) {
            directories = true;
        } else if (!strcmp(argv[index], "--dry-run") ||
                   !strcmp(argv[index], "-n")) {
            dryrun = true;
        } else if (!strcmp(argv[index], "--no-directories") ||
                   !strcmp(argv[index], "-D")) {
            directories = false;
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
        } else if (!strcmp(argv[index], "--no-statistics")) {
            statistics = false;
        } else if (!strcmp(argv[index], "--quiet") ||
                   !strcmp(argv[index], "-q")) {
            verbosity--;
        } else if (!strcmp(argv[index], "--statistics")) {
            statistics = true;
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

/* Add an item to a dependency set. */
static int add_from_string(depset_t *d, const char *cwd, char *path,
        filetype_t type) {
    char *absolute = abspath(cwd, path);
    if (absolute == NULL) {
        DEBUG("Failed to resolve path \"%s\"\n", path);
        return -1;
    }

    if (!directories) {
        struct stat st;
        /* Return without adding the file if this is a directory and we're not
         * tracking directories.
         */
        if (stat(absolute, &st) == 0 && (st.st_mode & S_IFDIR)) {
            IDEBUG("Skipping directory %s\n", absolute);
            free(absolute);
            return 0;
        }
    }

    for (unsigned int i = 0; i < exclude_sz; i++) {
        if (strncmp(exclude[i].prefix, absolute, exclude[i].length) == 0) {
            free(absolute);
            return 0;
        }
    }

    if (depset_add(d, absolute, type) != 0) {
        DEBUG("Failed to add %s \"%s\"\n",
            type == XC_INPUT ? "input" :
            type == XC_OUTPUT ? "output" :
            type == XC_AMBIGUOUS ? "ambiguous" : "unknown",
            absolute);
        free(absolute);
        return -1;
    }
    free(absolute);

    return 0;
}

static int add_from_reg(depset_t *d, syscall_t *syscall, int argno, filetype_t type) {
    char *filename = syscall_getstring(syscall, argno);
    if (filename == NULL) {
        if (syscall->call == SYS_execve) {
            /* A successful execve results in two entry SIGTRAPs, the
             * second one with an argument of NULL. Presumably the second
             * trap is an artefact of the program loader.
             */
            return 0;
        }
        DEBUG("Failed to retrieve string argument %d from syscall %s (%ld)\n",
            argno, translate_syscall(syscall->call), syscall->call);
        return -1;
    }

    int r = add_from_string(d, syscall->proc->cwd, filename, type);
    free(filename);
    return r;
}

static int add_from_fd_and_reg(depset_t *d, syscall_t *syscall, int fdarg,
        int argno, filetype_t type) {
    char *filename = syscall_getstring(syscall, argno);
    if (filename == NULL) {
        DEBUG("Failed to retrieve string argument %d from syscall %s (%ld)\n",
            argno, translate_syscall(syscall->call), syscall->call);
        return -1;
    }

    char *fdpath = syscall_getfd(syscall, fdarg);
    if (fdpath == NULL) {
        free(filename);
        DEBUG("Failed to retrieve file descriptor argument %d from syscall %s "
            "(%ld)\n", fdarg, translate_syscall(syscall->call), syscall->call);
        return -1;
    }

    normpath(fdpath, filename);
    free(filename);

    int r = add_from_string(d, syscall->proc->cwd, fdpath, type);
    free(fdpath);
    return r;
}

/* Sanity checks on open flags because checking for O_RDONLY is awkward. */
static_assert(O_RDONLY == 00 && O_WRONLY == 01 && O_RDWR == 02,
    "unexpected file open flag values");
/* See usage of this below. */
static const int FLAG_MASK = O_RDONLY | O_WRONLY | O_RDWR;

static int flags_to_mode(int flags) {
    return flags & FLAG_MASK;
}

/* Determine the type of an input or output we're opening based on the flags
 * passed. Note that we return XC_NONE unless this is an input because we're
 * only considering how we want to treat this on syscall entry.
 */
static filetype_t classify_open_entry(int flags) {
    int mode = flags_to_mode(flags);

    /* If we're opening this file write-only, we don't need to
     * do any measurement before opening as this file is purely
     * an output.
     */
    if (mode == O_WRONLY)
        return XC_NONE;

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
            return XC_NONE;

        /* If a file is opened with O_TRUNC, we're ignoring its
         * current contents and hence treating it purely as an
         * output. O_TRUNC actually has no effect if the file is
         * a device or a fifo, but regardless the caller is
         * clearly not expecting to depend on the existing
         * contents.
         */
        if (flags & O_TRUNC)
            return XC_NONE;

    }

    return XC_INPUT;
}

/* Determine how we want to treat an input or output we're opening on syscall
 * exit.
 */
static filetype_t classify_open_exit(int flags) {
    int mode = flags_to_mode(flags);
    if (mode == O_WRONLY || mode == O_RDWR)
        return XC_OUTPUT;

    return XC_NONE;
}

int main(int argc, char **argv) {
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

    cache_t *cache = cache_open(cache_dir, statistics);
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

    target_t target;
    if (trace(&target, &argv[index], hook_getenv ? argv[0] : NULL) != 0) {
        ERROR("Failed to start and trace target %s\n", argv[index]);
        return -1;
    }

    bool success = false;

    syscall_t *s;
    while ((s = next_syscall(&target)) != NULL) {

        /* Any syscall we receive may be the kernel entry or exit. Handle entry
         * separately first because there are relatively few syscalls where
         * entry is relevant for us. The relevant ones are essentially ones
         * that destroy some resource we need to measure before it disappears.
         */
        if (s->enter) {
            IDEBUG("trapped entry of %s from pid %u\n",
                translate_syscall(s->call), s->proc->pid);

            switch (s->call) {

                /* We need to handle execve on kernel entry because our
                 * original address space containing the input argument is gone
                 * on kernel exit.
                 */
                case SYS_execve:
                    if (add_from_reg(deps, s, 1, XC_INPUT) != 0)
                        goto bailout;
                    break;

                case SYS_open: {
                    /* In the case where a file is being opened RW, we need to
                     * do our measurement beforehand in case the user is using
                     * a flag like O_CREAT that makes measurement ambiguous
                     * when done afterwards. To simplify things, we handle RO
                     * open here as well.
                     */
                    int flags = (int)syscall_getarg(s, 2);
                    filetype_t type = classify_open_entry(flags);
                    if (type != XC_NONE)
                        if (add_from_reg(deps, s, 1, type) != 0)
                            goto bailout;
                    break;
                }

                case SYS_openat: {
                    /* As for open() but we need to handle prefixing from a file
                     * descriptor.
                     */
                    int flags = (int)syscall_getarg(s, 3);
                    filetype_t type = classify_open_entry(flags);
                    if (type != XC_NONE)
                        if (add_from_fd_and_reg(deps, s, 1, 2, type) != 0)
                            goto bailout;
                    break;
                }

                case SYS_rename:
                    if (add_from_reg(deps, s, 1, XC_INPUT) != 0)
                        goto bailout;
                    break;

                case SYS_unlink:
                    if (add_from_reg(deps, s, 1, XC_INPUT) != 0)
                        goto bailout;
                    break;

                case SYS_rmdir:
                    if (add_from_reg(deps, s, 1, XC_INPUT) != 0)
                        goto bailout;
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
            IDEBUG("resuming entry of %s for pid %u\n",
                translate_syscall(s->call), s->proc->pid);
            acknowledge_syscall(s);
            continue;
        }

        /* We should now only be handling syscall exits. */
        assert(!s->enter);

        IDEBUG("trapped exit of %s from pid %u\n", translate_syscall(s->call),
            s->proc->pid);

        switch (s->call) {

            case SYS_access:
                if (add_from_reg(deps, s, 1, XC_INPUT) != 0)
                    goto bailout;
                break;

            case SYS_chdir:
                if (s->result != 0) {
                    /* The target failed to change directory; no action
                     * required.
                     */
                    break;
                }
                /* Here, we could read the new working directory of the process
                 * out of the argument to their syscall, but rather than have
                 * to deal with complications involving relative paths we just
                 * do a standard update. If anything, this should be faster as
                 * we read directly from /proc instead of mmaping.
                 */
                if (proc_update_cwd(s->proc) != 0) {
                    DEBUG("bailing out due to failure to read current working "
                        "directory\n");
                    goto bailout;
                }
                break;

            case SYS_chmod:
                if (add_from_reg(deps, s, 1, XC_OUTPUT) != 0)
                    goto bailout;
                break;

            case SYS_creat:
                if (add_from_reg(deps, s, 1, XC_OUTPUT) != 0)
                    goto bailout;
                break;

            case SYS_open: {
                /* Note that we are only handling the 'write' aspects of an
                 * open call here, because the 'read' aspects were handled on
                 * syscall entry.
                 */
                int flags = (int)syscall_getarg(s, 2);
                filetype_t type = classify_open_exit(flags);
                if (type != XC_NONE)
                    if (add_from_reg(deps, s, 1, type) != 0)
                        goto bailout;
                break;
            }

            case SYS_openat: {
                int flags = (int)syscall_getarg(s, 3);
                filetype_t type = classify_open_exit(flags);
                if (type != XC_NONE)
                    if (add_from_fd_and_reg(deps, s, 1, 2, type) != 0)
                        goto bailout;
                break;
            }

            case SYS_readlink:
                if (add_from_reg(deps, s, 1, XC_INPUT) != 0)
                    goto bailout;
                break;

            case SYS_stat:
                if (add_from_reg(deps, s, 1, XC_AMBIGUOUS) != 0)
                    goto bailout;
                break;

            /* XXX: The syscalls that follow are known to be relevant, but are
             * not yet handled. Implement handlers for these on demand.
             */
            case SYS__sysctl:
            case SYS_acct:
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

        IDEBUG("resuming exit of %s for pid %u\n", translate_syscall(s->call),
            s->proc->pid);
        acknowledge_syscall(s);

    }

    /* If we've reached here (i.e. not jumped to 'bailout'), we successfully
     * traced the target. Hence we can now cache its dependency set for
     * retrieval on a later run.
     */
    success = true;

bailout:;

    int ret = complete(&target);

    const char *outfile = get_stdout(&target),
               *errfile = get_stderr(&target);

    if (success && ret == 0) {
        DEBUG("Adding cache entry\n");
        if (cache_write(cache, argc - index, &argv[index], deps, &target.env,
                outfile, errfile) != 0)
            /* This failure is non-critical in a sense. */
            DEBUG("Failed to write entry to cache\n");
    }

    depset_destroy(deps);

    if (outfile != NULL)
        unlink(outfile);
    if (errfile != NULL)
        unlink(errfile);

    cache_close(cache);
    delete(&target);
    return ret;
}
