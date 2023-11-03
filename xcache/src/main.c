#include "alloc.h"
#include "debug.h"
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>
#include <xcache/xcache.h>

static bool record_enabled = true;
static bool replay_enabled = true;

static char *cache_dir;

static xc_db_t *db;

static xc_cmd_t cmd;

bool debug;

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
        {"version", no_argument, 0, 'V'},
        {"wo", no_argument, 0, 133},
        {"write-only", no_argument, 0, 134},
        {0},
    };

    int index;
    int c = getopt_long(argc, argv, "d:h", opts, &index);

    if (c == -1)
      break;

    switch (c) {

    case 130: // --debug
      xc_set_debug(stderr);
      debug = true;
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

    case 'V': { // --version, -V
      printf("xcache version %s\n supported recording modes: ", xc_version());
      unsigned modes = xc_record_modes(XC_MODE_AUTO);
      if (modes == 0)
        printf("<none>");
      const char *separator = "";
      if (modes & XC_SYSCALL) {
        printf("ptrace");
        separator = ", ";
      }
      if (modes & XC_EARLY_SECCOMP) {
        printf("%sseccomp (early)", separator);
        separator = ", ";
      }
      if (modes & XC_LATE_SECCOMP)
        printf("%sseccomp (late)", separator);
      printf("\n");
      exit(EXIT_SUCCESS);
    }

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
    if (rc) {
      fprintf(stderr, "xc_cmd_new: %s\n", strerror(rc));
      return rc;
    }
  }

  return 0;
}

/// state used by `replay_callback`
typedef struct {
  int rc;     ///< any error code
  bool found; ///< did we find a trace to replay?
} replay_callback_t;

static int replay_callback(const xc_trace_t *trace, void *state) {

  // skip if trace is invalid
  if (!xc_trace_is_valid(trace))
    return 0;

  // attempt replay
  replay_callback_t *st = state;
  st->found = true;
  st->rc = xc_replay(trace);

  // either way, we are done
  return 1;
}

int main(int argc, char **argv) {

  int rc = 0;

  if ((rc = parse_args(argc, argv)))
    goto done;

  if (replay_enabled || record_enabled) {
    assert(cache_dir != NULL);

    // if the cache does not exist, create it
    if (mkdir(cache_dir, 0755) < 0 && errno != EEXIST) {
      rc = errno;
      fprintf(stderr, "failed to create cache directory %s: %s\n", cache_dir,
              strerror(rc));
      goto done;
    }

    // suffix the DB path to namespace different versions
    char *root = NULL;
    const char *version = xc_version();
    if (xc_version_is_release(version)) {
      xasprintf(&root, "%s/%s", cache_dir, version);
    } else {
      xasprintf(&root, "%s/dev", cache_dir);
    }
    if ((rc = xc_db_open(root, &db))) {
      free(root);
      fprintf(stderr, "xc_db_open: %s\n", strerror(rc));
      goto done;
    }
    free(root);
  }

  if (replay_enabled) {
    DEBUG("attempting replay");
    replay_callback_t state = {0};
    if ((rc = xc_trace_find(db, cmd, replay_callback, &state))) {
      fprintf(stderr, "xc_trace_find: %s\n", strerror(rc));
      goto done;
    }
    if (state.rc) {
      rc = state.rc;
      fprintf(stderr, "replay_callback: %s\n", strerror(rc));
      goto done;
    }
    if (state.found) {
      DEBUG("replay succeeded");
      goto done;
    }
    DEBUG("replay failed: no trace found to replay");
  }

  if (record_enabled) {
    DEBUG("attempting record");
    if ((rc = xc_record(db, cmd, XC_SYSCALL))) {
      fprintf(stderr, "xc_record: %s\n", strerror(rc));
      goto done;
    }
    DEBUG("record succeeded");
    goto done;
  }

  // did not replay or record, so exec uninstrumented
  DEBUG("running command uninstrumented");
  if ((rc = xc_cmd_exec(cmd))) {
    fprintf(stderr, "xc_cmd_exec: %s\n", strerror(rc));
    goto done;
  }

done:
  xc_db_close(db);
  xc_cmd_free(cmd);
  free(cache_dir);

  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
