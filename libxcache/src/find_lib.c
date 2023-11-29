#include "debug.h"
#include "find_me.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int find_lib(char **me) {

  assert(me != NULL);

  *me = NULL;
  char *maps = NULL;
  FILE *f = NULL;
  char *line = NULL;
  char *myself = NULL;
  int rc = 0;

  f = fopen("/proc/self/maps", "r");
  if (ERROR(f == NULL)) {
    rc = errno;
    goto done;
  }

  // read line-by-line, looking for ourselves
  size_t line_size = 0;
  while (true) {
    errno = 0;
    const ssize_t r = getline(&line, &line_size, f);
    if (r < 0) {
      if (ERROR(errno != 0)) {
        rc = errno;
        goto done;
      }
      // failed to find ourselves
      DEBUG("found no /proc/self/maps line for libxcache.so");
      rc = ENOENT;
      goto done;
    }

    size_t len = (size_t)r;

    // trim trailing newline to simplify later steps
    if (len > 0 && line[len - 1] == '\n') {
      line[len - 1] = '\0';
      --len;
    }

    // does this look like us?
    const char suffix[] = "/libxcache.so";
    if (strlen(line) < strlen(suffix))
      continue;
    if (strcmp(&line[len - strlen(suffix)], suffix) != 0)
      continue;

    // find the start of the full path
    size_t start = len - strlen(suffix);
    while (start > 0 && !isspace(line[start - 1]))
      --start;

    myself = strdup(&line[start]);
    if (myself == NULL) {
      rc = ENOMEM;
      goto done;
    }

    break;
  }

  // double check this actually exists
  if (ERROR(access(myself, F_OK) < 0)) {
    rc = errno;
    goto done;
  }

  *me = myself;
  myself = NULL;

done:
  free(myself);
  free(line);
  if (f != NULL)
    (void)fclose(f);
  free(maps);

  return rc;
}
