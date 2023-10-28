#include "alloc.h"
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <xcache/xcache.h>

static bool record_enabled = true;
static bool replay_enabled = true;

static char *cache_dir;

static xc_cmd_t cmd;

static int parse_args(int argc, char **argv) {

  while (true) {
    const struct option opts[] = {
        {"debug", no_argument, 0, 130},
        {"dir", required_argument, 0, 'd'},
        {"disable", no_argument, 0, 131},
        {"help", no_argument, 0, 'h'},
        {"read-only", no_argument, 0, 132},
        {"read-write", no_argument, 0, 133},
        {"ro", no_argument, 0, 131},
        {"rw", no_argument, 0, 132},
        {"wo", no_argument, 0, 133},
        {"write-only", no_argument, 0, 134},
        {0},
    };

    int index;
    int c = getopt_long(argc, argv, "d:h", opts, &index);

    if (c == -1)
      break;

    switch (c) {

    case 130:
      // TODO
      break;

    case 'd': // --dir, -d
      free(cache_dir);
      cache_dir = xstrdup(optarg);
      break;

    case 131: // --disable
      record_enabled = false;
      replay_enabled = false;
      break;

    case 'h':
      // TODO
      break;

    case 132: // --read-only, --ro
      record_enabled = false;
      replay_enabled = true;
      break;

    case 133: // --read-write, --rw
      record_enabled = true;
      replay_enabled = true;
      break;

    case 134: // --write-only, --wo
      record_enabled = true;
      replay_enabled = false;
      break;

    default:
      exit(EX_USAGE);
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

  assert(argc >= optind);
  {
    int rc = xc_cmd_new(&cmd, (size_t)(argc - optind), &argv[optind], NULL);
    fprintf(stderr, "xc_cmd_new: %s\n", strerror(rc));
    return rc;
  }

  return 0;
}

int main(int argc, char **argv) {

  int rc = EXIT_FAILURE;

  if (parse_args(argc, argv) < 0)
    goto done;

  if (replay_enabled) {
    // TODO: find trace and exec
  }

  if (record_enabled) {
    // TODO: trace
  }

  // did not replay or record, so exec uninstrumented
  {
    int r = xc_cmd_exec(cmd);
    if (r) {
      fprintf(stderr, "xc_cmd_exec: %s\n", strerror(r));
      goto done;
    }
  }

  rc = EXIT_SUCCESS;
done:
  xc_cmd_free(cmd);
  free(cache_dir);

  return rc;
}
