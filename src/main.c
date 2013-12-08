#include <assert.h>
#include "config.h"
#include "depset.h"
#include "log.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include "trace.h"

static void usage(const char *prog) {
    fprintf(stderr, "%s [--debug | -d] [--help | -h] [--version] command args...\n", prog);
}

int main(int argc, char **argv) {
    int index;

    /* Parse command-line arguments. Unfortunately getopt has some undesirable
     * behaviour that prevents us using it.
     */
    for (index = 1; index < argc; index++) {
        if (!strcmp(argv[index], "--debug") ||
            !strcmp(argv[index], "-d")) {
            verbosity = L_DEBUG;
        } else if (!strcmp(argv[index], "--quiet") ||
                   !strcmp(argv[index], "-q")) {
            verbosity = L_QUIET;
        } else if (!strcmp(argv[index], "--version")) {
            printf("xcache %d.%02d\n", VERSION_MAJOR, VERSION_MINOR);
            return 0;
        } else if (!strcmp(argv[index], "--help") ||
                   !strcmp(argv[index], "-?")) {
            usage(argv[0]);
            return 0;
        } else {
            break;
        }
    }
    if (argc - index == 0) {
        DEBUG("no target command supplied\n");
        usage(argv[0]);
        return -1;
    }

    depset_t *deps = depset_new();
    if (deps == NULL) {
        ERROR("failed to create dependency set\n");
        return -1;
    }

    proc_t *target = trace(&argv[index]);
    if (target == NULL) {
        ERROR("failed to start and trace target %s\n", argv[index]);
        return -1;
    }

    syscall_t s;
    while (next_syscall(target, &s) == 0) {

#define ADD_AS(category, argno) \
    do { \
        char *_f = syscall_getstring(target, argno); \
        if (_f == NULL) goto bailout; \
        int _r = depset_add_##category(deps, _f); \
        if (_r != 0) goto bailout; \
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
                    DEBUG("bailing out due to unhandled syscall %ld\n", s.call);
                    goto bailout;

                default:
                    DEBUG("irrelevant syscall entry %ld\n", s.call);
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
            case SYS_open:
            case SYS_openat:
            case SYS_pivot_root:
            case SYS_readlink:
            case SYS_readlinkat:
            case SYS_rename:
            case SYS_renameat:
            case SYS_rmdir:
            case SYS_stat:
            case SYS_statfs:
            case SYS_swapoff:
            case SYS_swapon:
            case SYS_symlink:
            case SYS_symlinkat:
            case SYS_truncate:
            case SYS_unlink:
            case SYS_unlinkat:
#if __WORDSIZE == 32
            case SYS_umount:
#endif
            case SYS_umount2:
            case SYS_uselib:
                DEBUG("bailing out due to unhandled syscall %ld\n", s.call);
                goto bailout;

            default:
                DEBUG("irrelevant syscall exit %ld\n", s.call);
        }
        acknowledge_syscall(target);

#undef ADD_AS

    }

    int ret;
bailout:

    ret = complete(target);
    detach(target);
    return ret;
}

