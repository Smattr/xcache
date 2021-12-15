#include "debug.h"
#include "fs_set.h"
#include "macros.h"
#include "path.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int see_access(tracee_t *tracee, const char *pathname) {

  assert(tracee != NULL);
  assert(pathname != NULL);

  DEBUG("PID %d called access(\"%s\")", (int)tracee->pid, pathname);

  int rc = 0;

  // if `pathname` is not absolute, it is relative to the tracee’s cwd
  char *abs;
  if (pathname[0] == '/') {
    abs = strdup(pathname);
  } else {
    abs = path_join(tracee->cwd, pathname);
  }
  if (UNLIKELY(abs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // record a read for this file
  rc = fs_set_add_read(&tracee->trace.io, abs);
  if (UNLIKELY(rc != 0))
    goto done;

done:
  free(abs);

  return rc;
}
