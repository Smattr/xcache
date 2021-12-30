#include "../../libxcache/src/macros.h"
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcache/xcache.h>

/// allow recording new process executions into the database?
static bool enable_record = true;

/// allow replaying past process executions from the database?
static bool enable_replay = true;

/// root for the cache database?
static char *cache_dir;

static char *xstrdup(const char *s) {
  char *copy = strdup(s);
  if (UNLIKELY(copy == NULL)) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  return copy;
}

#define xasprintf(args...)                                                     \
  do {                                                                         \
    if (UNLIKELY(asprintf(args) < 0)) {                                        \
      fprintf(stderr, "out of memory\n");                                      \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

/// parse command line arguments and return the index of the first argument not
/// intended for us
static int parse_args(int argc, char **argv) {

  while (true) {
    static const struct option opts[] = {
        // clang-format off
        {"debug", no_argument, 0, 130},
        {"dir", required_argument, 0, 'd'},
        {"disable-record", no_argument, 0, 131},
        {"disable-replay", no_argument, 0, 132},
        {"enable-record", no_argument, 0, 133},
        {"enable-replay", no_argument, 0, 134},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
        // clang-format on
    };

    int index;
    int c = getopt_long(argc, argv, "d:h", opts, &index);

    if (c == -1)
      break;

    switch (c) {

    case 130: // --debug
      xc_set_debug(stderr);
      break;

    case 'd': // --dir
      free(cache_dir);
      cache_dir = xstrdup(optarg);
      break;

    case 131: // --disable-record
      enable_record = false;
      break;

    case 132: // --disable-replay
      enable_replay = false;
      break;

    case 133: // --enable-record
      enable_record = true;
      break;

    case 134: // --enable-replay
      enable_replay = true;
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

  // if `--dir` was not given, use environment variables to decide
  if (cache_dir == NULL) {
    const char *XCACHE_DIR = getenv("XCACHE_DIR");
    if (XCACHE_DIR != NULL)
      cache_dir = xstrdup(XCACHE_DIR);
  }
  if (cache_dir == NULL) {
    const char *XDG_CACHE_HOME = getenv("XDG_CACHE_HOME");
    if (XDG_CACHE_HOME != NULL)
      xasprintf(&cache_dir, "%s/xcache", XDG_CACHE_HOME);
  }
  if (cache_dir == NULL) {
    const char *HOME = getenv("HOME");
    if (HOME == NULL) {
      fprintf(stderr, "--dir not provided and none of $XCACHE_DIR, "
                      "$XDG_CACHE_HOME, $HOME set\n");
      exit(EXIT_FAILURE);
    }
    xasprintf(&cache_dir, "%s/.xcache", HOME);
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

  int rc = 0;

  // construct the process to record/replay
  xc_proc_t *proc = NULL;
  rc = make_process(&proc, argc - arg0, argv + arg0);
  if (UNLIKELY(rc != 0)) {
    fprintf(stderr, "failed to create process: %s\n", strerror(rc));
    goto done;
  }

  if (enable_replay) {
    // TODO: detect if the child can be replayed

  }

  if (enable_record) {
#if 0 // TODO
  // record child execution
  xc_trace_t *trace = NULL;
  rc = xc_trace_record(&trace, proc);
  if (UNLIKELY(rc != 0)) {
    fprintf(stderr, "failed to record process: %s\n", strerror(rc));
    goto done;
  }
#endif
  }

  // else fall back on pass through execution
  rc = xc_proc_exec(proc);

done:
  xc_proc_free(proc);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
