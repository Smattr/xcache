#include "debug.h"
#include "macros.h"
#include "path.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/hash.h>

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

  // derive a hash of this (possible) file
  xc_hash_t h = 0;
  int r = xc_hash_file(&h, abs);
  bool exists = r != ENOENT;
  bool allowed = r != EACCES && r != EPERM;
  // TODO: do we need to care about the path being a directory?
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
