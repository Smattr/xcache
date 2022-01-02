#include "../../libxcache/src/macros.h"
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcache/xcache.h>

/// are we in debug mode?
static bool debug = false;

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

/// our own equivalent of ../../libxcache/src/debug.h:DEBUG
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(debug)) {                                                     \
      const char *name_ = strrchr(__FILE__, '/');                              \
      fprintf(stderr, "xcache:xcache/src%s:%d: [DEBUG] ", name_, __LINE__);    \
      fprintf(stderr, args);                                                   \
      fprintf(stderr, "\n");                                                   \
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
      debug = true;
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

static int replay(const xc_db_t *db, const xc_proc_t *proc, int *exit_status) {

  // try to find a prior trace in the cache
  xc_trace_t *trace = NULL;
  int rc = xc_db_load(db, proc, &trace);
  if (rc != 0)
    goto done;

  // is the trace still viable?
  if (!xc_trace_is_valid(trace)) {
    rc = ENOENT;
    goto done;
  }

  // try to replay it
  rc = xc_trace_replay(trace);
  if (UNLIKELY(rc != 0))
    goto done;

  assert(trace != NULL);
  *exit_status = xc_trace_exit_status(trace);

done:
  xc_trace_free(trace);

  return rc;
}

static int record(xc_db_t *db, const xc_proc_t *proc, int *exit_status) {

  // try to run the process and record its trace
  xc_trace_t *trace = NULL;
  int rc = xc_trace_record(&trace, proc, db);
  if (UNLIKELY(rc != 0))
    goto done;

  // save the trace for a future execution
  rc = xc_db_save(db, proc, trace);
  if (UNLIKELY(rc != 0))
    goto done;

done:
  if (rc == 0 || rc == ENOTSUP) {
    assert(trace != NULL);
    *exit_status = xc_trace_exit_status(trace);
  }

  xc_trace_free(trace);

  return rc;
}

int main(int argc, char **argv) {

  // parse command line arguments
  int arg0 = parse_args(argc, argv);

  xc_proc_t *proc = NULL;
  xc_db_t *db = NULL;
  int exit_status = EXIT_FAILURE;
  int rc = -1;

  // did the caller give us too few arguments?
  if (UNLIKELY(arg0 >= argc)) {
    fprintf(stderr, "no program provided for xcache to run\n");
    goto done;
  }

  // construct the process to record/replay
  rc = make_process(&proc, argc - arg0, argv + arg0);
  if (UNLIKELY(rc != 0)) {
    fprintf(stderr, "failed to create process: %s\n", strerror(rc));
    goto done;
  }

  // open the database, if relevant
  if (enable_replay || enable_record) {
    rc = xc_db_open(&db, cache_dir);
    if (UNLIKELY(rc != 0)) {
      fprintf(stderr, "failed to open database: %s\n", strerror(rc));
      goto done;
    }
  }

  // try to find and replay a prior trace
  if (enable_replay) {
    assert(db != NULL);

    rc = replay(db, proc, &exit_status);
    DEBUG("replay %s", rc == 0 ? "succeeded" : "failed");
    if (UNLIKELY(rc != 0 && rc != ENOENT)) {
      fprintf(stderr, "trace replay failed: %s\n", strerror(rc));
      goto done;
    }

    // if the replay was successful, we are done
    if (rc == 0)
      goto done;
  }

  if (enable_record) {
    assert(db != NULL);

    // record child execution
    rc = record(db, proc, &exit_status);
    DEBUG("record %s", rc == 0 ? "succeeded" : "failed");

    // if we bailed out due to an unsupported system call, we are done
    if (rc == ENOTSUP)
      goto done;

    if (UNLIKELY(rc != 0)) {
      fprintf(stderr, "trace record failed: %s\n", strerror(rc));
      goto done;
    }

    // if the record was successful, we are done
    if (rc == 0)
      goto done;
  }

  // else fall back on pass through execution
  assert(exit_status == EXIT_FAILURE && "exit status incorrectly modified");
  rc = xc_proc_exec(proc);

done:
  xc_db_close(db);
  xc_proc_free(proc);
  free(cache_dir);

  return exit_status;
}
