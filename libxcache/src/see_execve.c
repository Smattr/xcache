#include "debug.h"
#include "macros.h"
#include "path.h"
#include "trace.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/hash.h>

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

  // derive a hash of this file
  xc_hash_t h = 0;
  int r = xc_hash_file(&h, abs);
  bool exists = r != ENOENT;
  bool allowed = r != EACCES && r != EPERM;
  if (UNLIKELY(exists && allowed && r != 0)) {
    rc = r;
    goto done;
  }

  // append this to our collection of read files
  read_file f = {.path = abs, .hash = h};
  rc = trace_append_read(&tracee->trace, f);
  if (UNLIKELY(rc != 0))
    goto done;

done:
  if (UNLIKELY(rc != 0))
    free(abs);

  return rc;
}
