#include "db_t.h"
#include "debug.h"
#include "hash_t.h"
#include "path.h"
#include "trace_t.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <xcache/cmd.h>
#include <xcache/db.h>
#include <xcache/trace.h>

static bool endswith(const char *s, const char *suffix) {
  const size_t len1 = strlen(s);
  const size_t len2 = strlen(suffix);
  if (len1 < len2)
    return false;
  return strcmp(&s[len1 - len2], suffix) == 0;
}

int xc_trace_find(const xc_db_t *db, const xc_cmd_t query,
                  int (*cb)(const xc_trace_t *trace, void *state),
                  void *state) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(db->root == NULL))
    return EINVAL;

  if (ERROR(query.argc == 0))
    return EINVAL;

  if (ERROR(query.argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < query.argc; ++i) {
    if (ERROR(query.argv[i] == NULL))
      return EINVAL;
  }

  if (ERROR(query.cwd == NULL))
    return EINVAL;

  DIR *dir = NULL;
  char *trace_root = NULL;
  int rc = 0;

  if (ERROR((rc = db_trace_root(*db, query, &trace_root))))
    goto done;

  dir = opendir(trace_root);
  if (dir == NULL) {
    // allow the trace to not exist
    if (ERROR(errno != ENOENT)) {
      rc = errno;
      goto done;
    }
    goto done;
  }

  while (true) {

    errno = 0;
    const struct dirent *entry = readdir(dir);
    if (entry == NULL) {
      if (ERROR(errno != 0)) {
        rc = errno;
        goto done;
      }
      break;
    }

    if (entry->d_type != DT_REG)
      continue;

    if (!endswith(entry->d_name, ".trace"))
      continue;

    char *trace_file = path_join(trace_root, entry->d_name);
    if (ERROR(trace_file == NULL)) {
      rc = ENOMEM;
      goto done;
    }

    int trace_fd = open(trace_file, O_RDONLY | O_CLOEXEC);
    if (ERROR(trace_fd < 0)) {
      rc = errno;
      free(trace_file);
      goto done;
    }
    free(trace_file);
    FILE *trace_f = fdopen(trace_fd, "r");
    if (ERROR(trace_f == NULL)) {
      rc = errno;
      (void)close(trace_fd);
      goto done;
    }

    xc_trace_t trace = {0};
    rc = trace_read(&trace, trace_f);
    (void)fclose(trace_f);
    if (ERROR(rc))
      goto done;

    if (!xc_cmd_eq(query, trace.cmd)) {
      xc_trace_free(&trace);
      continue;
    }

    const int r = cb(&trace, state);
    xc_trace_free(&trace);
    if (r != 0)
      break;
  }

done:
  if (dir != NULL)
    (void)closedir(dir);
  free(trace_root);

  return rc;
}
