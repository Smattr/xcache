#include "debug.h"
#include "fs_set.h"
#include "macros.h"
#include "path.h"
#include "trace.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int see_execve(tracee_t *tracee, const char *filename) {

  assert(filename != NULL);

  DEBUG("PID %d called execve(\"%s\")", (int)tracee->pid, filename);

  // note that we do not care whether the `execve` succeeded or not; either way,
  // we count it as a read

  int rc = 0;

  // if filename is not absolute, it is relative to the tracee’s cwd
  char *abs;
  if (filename[0] == '/') {
    abs = strdup(filename);
  } else {
    abs = path_join(tracee->cwd, filename);
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
