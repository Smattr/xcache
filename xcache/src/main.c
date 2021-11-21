#include "../../libxcache/src/macros.h"
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcache/xcache.h>

/// parse command line arguments and return the index of the first argument not
/// intended for us
static int parse_args(int argc, char **argv) {

  while (true) {
    static const struct option opts[] = {
        // clang-format off
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
        // clang-format on
    };

    int index;
    int c = getopt_long(argc, argv, "dh", opts, &index);

    if (c == -1)
      break;

    switch (c) {

    case 'd': // --debug
      xc_set_debug(stderr);
      break;

    case 'h': // --help
      printf("%s options\n"
             "  ccache for everything\n",
             argv[0]);
      exit(EXIT_SUCCESS);

    default:
      exit(EXIT_FAILURE);
    }
  }

  return optind;
}

/// construct a new process object we intend to record or replay
static int make_process(xc_proc_t **proc, int argc, char **argv) {

  // what directory are we running in?
  char *cwd = getcwd(NULL, 0);
  if (UNLIKELY(cwd == NULL))
    return errno;

  // construct the process to record/replay
  int rc = xc_proc_new(proc, argc, argv, cwd);
  free(cwd);

  return rc;
}

int main(int argc, char **argv) {

  // parse command line arguments
  int arg0 = parse_args(argc, argv);

  // did the caller give us too few arguments?
  if (UNLIKELY(arg0 >= argc)) {
    fprintf(stderr, "no program provided for xcache to run\n");
    return EXIT_FAILURE;
  }

  // construct the process to record/replay
  xc_proc_t *proc = NULL;
  int rc = make_process(&proc, argc - arg0, argv + arg0);
  if (UNLIKELY(rc != 0)) {
    fprintf(stderr, "failed to create process: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  // TODO: attempt to record or replay the process
  rc = xc_proc_exec(proc);

  xc_proc_free(proc);

  if (UNLIKELY(rc != 0)) {
    fprintf(stderr, "process execution failed: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
