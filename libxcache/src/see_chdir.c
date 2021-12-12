#include "debug.h"
#include "macros.h"
#include "path.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int see_chdir(tracee_t *tracee, int result, const char *path) {

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
  char *p = path_join(tracee->cwd, path);
  if (UNLIKELY(p == NULL))
    return ENOMEM;

  // update the tracee’s cwd
  free(tracee->cwd);
  tracee->cwd = p;

  return 0;
}
