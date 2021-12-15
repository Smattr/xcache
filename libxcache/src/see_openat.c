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
    DEBUG("PID %d called openat(%d, \"%s\", %s | %d), ret %ld", (int)tracee->pid,
          dirfd, pathname, flag_name(flags), other_flags(flags), result);
  }

  // TODO: should a failed openat be considered a read?
  assert(result >= 0 && "TODO");

  int rc = 0;

  // turn the path into something absolute
  char *abs = NULL;
  if (pathname[0] == '/') {
    // it is already absolute
    abs = strdup(pathname);
  } else if (dirfd == AT_FDCWD) {
    abs = path_join(tracee->cwd, pathname);
  } else {
    assert(0 && "TODO");
  }
  if (UNLIKELY(abs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (flags & O_WRONLY) {
    assert(0 && "TODO");
  } else if (flags & O_RDWR) {
    assert(0 && "TODO");
  } else {
    // O_RDONLY

    // record a read for this file
    rc = fs_set_add_read(&tracee->trace.io, abs);
    if (UNLIKELY(rc != 0))
      goto done;
  }

done:
  free(abs);

  return rc;
}
