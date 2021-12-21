#include "debug.h"
#include "fs_set.h"
#include "macros.h"
#include "path.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int see_openat_middle(tracee_t *tracee, int dirfd, const char *pathname) {

  assert(tracee != NULL);
  assert(pathname != NULL);

  if (dirfd == AT_FDCWD) {
    DEBUG("PID %d called openat(AT_FDCWD, \"%s\", …)", (int)tracee->pid,
          pathname);
  } else {
    DEBUG("PID %d called openat(%d, \"%s\", …)", (int)tracee->pid, dirfd,
          pathname);
  }

  int rc = 0;

  // turn the path into something absolute
  char *abs = NULL;
  if (pathname[0] == '/') {
    // it is already absolute
    abs = strdup(pathname);
  } else if (dirfd == AT_FDCWD) {
    abs = path_join(tracee->cwd, pathname);
  } else {

    // find the handle this is relative to
    open_file_t *of = tracee->fds;
    while (of != NULL && of->handle != dirfd)
      of = of->next;

    // was this `openat` call passed an invalid handle?
    if (of == NULL)
      goto done;

    abs = path_join(of->path, pathname);
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
