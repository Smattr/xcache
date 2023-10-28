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
        {"dry-run", no_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {"read-only", no_argument, 0, 131},
        {"read-write", no_argument, 0, 132},
        {"ro", no_argument, 0, 131},
        {"rw", no_argument, 0, 132},
        {"wo", no_argument, 0, 133},
        {"write-only", no_argument, 0, 133},
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

    case 'n': // --dry-run, -n
      record_enabled = false;
      replay_enabled = false;
      break;

    case 'h':
      // TODO
      break;

    case 131: // --read-only, --ro
      record_enabled = false;
      replay_enabled = true;
      break;

    case 132: // --read-write, --rw
      record_enabled = true;
      replay_enabled = true;
      break;

    case 133: // --write-only, --wo
      record_enabled = true;
      replay_enabled = false;
      break;

    default:
      exit(EX_USAGE);
    }
  }

  assert(argc >= optind);
  cmd.argc = (size_t)(argc - optind);
  cmd.argv = &argv[optind];

  cmd.cwd = getcwd(NULL, 0);
  if (cmd.cwd == NULL) {
    int rc = errno;
    fprintf(stderr, "getcwd: %s\n", strerror(rc));
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
  free(cmd.cwd);

  return rc;
}
