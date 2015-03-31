/* Debugging stub program for xcache.
 *
 * This program uses libxcache, but instead of performing any caching it just
 * traps and notes every syscall. The purpose of this is so we can investigate
 * inauthentic xcache mimicking by tracing a command under this instead and
 * rule out any extraneous xcache maneuvering as the cause.
 */

#include <errno.h>
#include "log.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "trace.h"
#include "translate-syscall.h"
#include <unistd.h>

static bool hook_getenv = true;

static void usage(const char *prog) {
    fprintf(stderr, "Usage:\n"
        "  %s [options] command args...\n"
        "\n"
        "Options:\n"
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

static int parse_arguments(int argc, char **argv) {
    int index;
    for (index = 1; index < argc; index++) {
        if (!strcmp(argv[index], "--help") ||
            !strcmp(argv[index], "-?")) {
            usage(argv[0]);
            exit(0);
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
            printf("oversee %d.%02d\n", VERSION_MAJOR, VERSION_MINOR);
            exit(0);
        } else {
            break;
        }
    }
    return index;
}

int main(int argc, char **argv) {
    int index = parse_arguments(argc, argv);

    target_t t;
    if (trace(&t, &argv[index], hook_getenv ? argv[0] : NULL) != 0) {
        ERROR("failed to start and trace target %s\n", argv[index]);
        return -1;
    }

    syscall_t *s;
    while ((s = next_syscall(&t)) != NULL) {
        INFO("%s %s from pid %u\n", translate_syscall(s->call),
            s->enter ? "enter" : "exit", s->proc->pid);
        acknowledge_syscall(s);
    }

    int ret = complete(&t);

    delete(&t);

    return ret;
}
