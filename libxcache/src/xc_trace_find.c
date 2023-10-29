#include "db_t.h"
#include "debug.h"
#include "hash_t.h"
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

  if (ERROR(db->root < 0))
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
  char path[sizeof(hash_t) * 2 + 1] = {0};
  int rc = 0;

  {
    // hash the command
    hash_t hash = {0};
    if (ERROR((rc = hash_cmd(query, &hash))))
      goto done;

    // stringise the hash
    snprintf(path, sizeof(path), "%016" PRIx64, hash.data);

    // there seems to be no `opendirat`, so take the long way around
    int dirfd = openat(db->root, path, O_RDONLY | O_CLOEXEC | O_DIRECTORY);
    if (ERROR(dirfd < 0)) {
      rc = errno;
      goto done;
    }
    dir = fdopendir(dirfd);
    if (ERROR(dir == NULL)) {
      rc = errno;
      (void)close(dirfd);
      goto done;
    }
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

    const size_t leaf_len = strlen(entry->d_name);
    char *stem = calloc(1, sizeof(path) + sizeof("/") - 1 + leaf_len);
    if (ERROR(stem == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    memcpy(stem, path, sizeof(path) - 1);
    stem[sizeof(path) - 1] = '/';
    memcpy(&stem[sizeof(path) - 1 + sizeof("/") - 1], entry->d_name, leaf_len);

    int trace_fd = openat(db->root, stem, O_RDONLY | O_CLOEXEC);
    if (ERROR(trace_fd < 0)) {
      rc = errno;
      free(stem);
      goto done;
    }
    // TODO: lock
    free(stem);
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

    const int r = cb(&trace, state);
    xc_trace_free(&trace);
    if (r != 0)
      break;
  }

done:
  if (dir != NULL)
    (void)closedir(dir);

  return rc;
}
