#include "debug.h"
#include "path.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static char *make_absolute(tracee_t *tracee, int dirfd, const char *pathname) {

  assert(tracee != NULL);
  assert(pathname != NULL);

  // is the path already absolute?
  if (pathname[0] == '/')
    return strdup(pathname);

  // is it relative to the current directory?
  if (dirfd == AT_FDCWD)
    return path_join(tracee->cwd, pathname);

  // find the handle this is relative to
  open_file_t *of = tracee->fds;
  while (of != NULL && of->handle != dirfd)
    of = of->next;
  assert(
      of != NULL &&
      "tracee successfully opened something relative to a non-existent FD");

  return path_join(of->path, pathname);
}

int see_read(tracee_t *tracee, int dirfd, const char *pathname) {

  assert(tracee != NULL);
  assert(pathname != NULL);

  // Ignore reads to std handles. Note this is only valid as long as we do not
  // support `dup2`.
  if (pathname[0] != '/') {
    if (dirfd == STDIN_FILENO || dirfd == STDOUT_FILENO || dirfd == STDERR_FILENO) {
      DEBUG("ignoring read relative to std fd %d", dirfd);
      return 0;
    }
  }

  // TODO: check dirfd is valid and bail if not

  int rc = 0;

  // turn the path into something absolute
  char *abs = make_absolute(tracee, dirfd, pathname);
  if (ERROR(abs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // record a read for this file
  rc = fs_set_add_read(&tracee->trace.io, abs);
  if (ERROR(rc != 0))
    goto done;

done:
  free(abs);

  return rc;
}

int see_write(tracee_t *tracee, int dirfd, const char *pathname) {

  assert(tracee != NULL);
  assert(pathname != NULL);

  // Ignore writes to std handles. Note this is only valid as long as we do not
  // support `dup2`.
  if (pathname[0] != '/') {
    if (dirfd == STDIN_FILENO || dirfd == STDOUT_FILENO || dirfd == STDERR_FILENO) {
      DEBUG("ignoring read relative to std fd %d", dirfd);
      return 0;
    }
  }

  int rc = 0;

  // turn the path into something absolute
  char *abs = make_absolute(tracee, dirfd, pathname);
  if (ERROR(abs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // record a write for this file
  rc = fs_set_add_write(&tracee->trace.io, abs);
  if (ERROR(rc != 0))
    goto done;

done:
  free(abs);

  return rc;
}

int see_open(tracee_t *tracee, long result, int dirfd, const char *pathname) {

  assert(tracee != NULL);
  assert(pathname != NULL);
  assert(result >= 0 && "failed open passed to see_open");

  int rc = 0;

  // turn the path into something absolute
  char *abs = make_absolute(tracee, dirfd, pathname);
  if (ERROR(abs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // save this handle in case some future operation does `openat` with respect
  // to it
  for (open_file_t **of = &tracee->fds;; of = &(*of)->next) {
    if (*of == NULL) {
      open_file_t *new = calloc(1, sizeof(*new));
      if (ERROR(new == NULL)) {
        rc = ENOMEM;
        goto done;
      }
      new->handle = result;
      new->path = abs;
      abs = NULL;
      new->next = tracee->fds;
      tracee->fds = new;
      break;
    }
    if ((*of)->handle == result) {
      // this handle must have been previously closed and is now recycled
      free((*of)->path);
      (*of)->path = abs;
      abs = NULL;
      break;
    }
  }

done:
  free(abs);

  return rc;
}
