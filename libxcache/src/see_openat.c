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

static const char *flag_name(int flags) {
  if (flags & O_WRONLY)
    return "O_WRONLY";
  if (flags & O_RDWR)
    return "O_RDWR";
  return "O_RDONLY";
}

static int other_flags(int flags) {
  return flags & ~(O_WRONLY | O_RDWR | O_RDONLY);
}

int see_openat(tracee_t *tracee, long result, int dirfd, const char *pathname,
               int flags) {

  assert(tracee != NULL);
  assert(pathname != NULL);

  if (dirfd == AT_FDCWD) {
    DEBUG("PID %d called openat(AT_FDCWD, \"%s\", %s | %d), ret %ld",
          (int)tracee->pid, pathname, flag_name(flags), other_flags(flags),
          result);
  } else {
    DEBUG("PID %d called openat(%d, \"%s\", %s | %d), ret %ld",
          (int)tracee->pid, dirfd, pathname, flag_name(flags),
          other_flags(flags), result);
  }

  // if this syscall failed, we can ignore it because at most it counted as a
  // read intent which was handled during `syscall_middle`
  if (result < 0)
    return 0;

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
    assert(
        of != NULL &&
        "tracee successfully opened something relative to a non-existent FD");

    abs = path_join(of->path, pathname);
  }
  if (UNLIKELY(abs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // reads were noted during `syscall_middle`, but we need to note writes now
  if ((flags & O_WRONLY) || (flags & O_RDWR)) {
    rc = fs_set_add_write(&tracee->trace.io, abs);
    if (UNLIKELY(rc != 0))
      goto done;
  }

done:
  free(abs);

  return rc;
}
