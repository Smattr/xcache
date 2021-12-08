#include "debug.h"
#include "macros.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int witness_chdir(tracee_t *tracee, int result, const char *path) {

  assert(tracee != NULL);
  assert(path != NULL);

  DEBUG("PID %d called chdir(\"%s\"), ret %d", (int)tracee->pid, path, result);

  // if this failed, then ignore it
  // TODO: should we consider a failed chdir a read?
  if (result != 0)
    return 0;

  // if this was an absolute path, use it as-is
  if (path[0] == '/') {
    free(tracee->cwd);
    tracee->cwd = strdup(path);
    if (UNLIKELY(tracee->cwd == NULL))
      return ENOMEM;

    return 0;
  }

  // otherwise, create an absolute path
  bool needs_slash = tracee->cwd[strlen(tracee->cwd) - 1] != '/';
  size_t len = strlen(tracee->cwd) + strlen(path) + 1;
  if (needs_slash)
    ++len;
  char *p = malloc(len);
  if (UNLIKELY(p == NULL))
    return ENOMEM;
  snprintf(p, len, "%s%s%s", tracee->cwd, needs_slash ? "/" : "", path);

  // update the tracee’s cwd
  free(tracee->cwd);
  tracee->cwd = p;

  return 0;
}
