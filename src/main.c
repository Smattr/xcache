#include "debug.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include "trace.h"

static void usage(const char *prog) {
    fprintf(stderr, "%s [--debug] command args...\n", prog);
}

int main(int argc, char **argv) {
    int index;

    for (index = 1; index < argc; index++) {
        if (!strcmp(argv[index], "--debug")) {
            debug = true;
        } else {
            break;
        }
    }
    if (argc - index == 0) {
        usage(argv[0]);
        return -1;
    }

    proc_t *target = trace(&argv[index]);
    if (target == NULL) {
        return -1;
    }

    syscall_t s;
    while (next_syscall(target, &s) == 0) {
        switch (s.call) {
            case SYS__sysctl:
            case SYS_access:
            case SYS_acct:
            case SYS_chdir:
            case SYS_chmod:
            case SYS_chown:
            case SYS_chroot:
            case SYS_creat:
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
                goto break2;

            default:
                printf("%s: %ld %ld\n", s.enter ? "call" : "return", s.call, s.result);
        }
        acknowledge_syscall(target);
    }

    int ret;
break2:

    ret = complete(target);
    detach(target);
    return ret;
}

