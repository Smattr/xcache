#include "debug.h"
#include "macros.h"
#include "path.h"
#include "peek.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
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

int syscall_end(tracee_t *tracee) {

  assert(tracee != NULL);

  // retrieve the syscall number
  long nr = peek_syscall_no(tracee->pid);

  // retrieve the result of the syscall
  long ret = peek_reg(tracee->pid, REG(rax));

  switch (nr) {

  case __NR_open: {

    // if this call failed, at most it counts as a read attempt that would have
    // been handled during `syscall_middle`
    if (ret < 0)
      return 0;

    // retrieve the path
    char *path = NULL;
    int rc = peek_string(&path, tracee->pid, REG(rdi));
    if (ERROR(rc != 0))
      return rc;

    // retrieve the flags
    int flags = peek_reg(tracee->pid, REG(rsi));

    DEBUG("PID %d called open(\"%s\", %s | %d), ret %ld", (int)tracee->pid,
          path, flag_name(flags), other_flags(flags), ret);

    // does this count as a write intent?
    bool is_write =
        (flags & O_RDWR) == O_RDWR || (flags & O_WRONLY) == O_WRONLY;
    if (is_write) {
      rc = see_write(tracee, AT_FDCWD, path);
      if (ERROR(rc != 0)) {
        free(path);
        return rc;
      }
    }

    // note the new file handle we need to learn
    rc = see_open(tracee, ret, AT_FDCWD, path);
    free(path);
    if (ERROR(rc != 0))
      return rc;

    return rc;
  }

  case __NR_chdir: {

    // if this call failed, nothing to do
    if (ret < 0)
      return 0;

    // retrieve the path
    char *path = NULL;
    int rc = peek_string(&path, tracee->pid, REG(rdi));
    if (ERROR(rc != 0))
      return rc;

    DEBUG("PID %d called chdir(\"%s\"), ret %ld", (int)tracee->pid, path, ret);

    // turn it into an absolute path
    char *cwd = NULL;
    if (path[0] == '/') { // already absolute?
      cwd = path;
    } else {
      cwd = path_join(tracee->cwd, path);
      free(path);
      if (ERROR(cwd == NULL))
        return ENOMEM;
    }

    free(tracee->cwd);
    tracee->cwd = cwd;

    return rc;
  }

  case __NR_openat: {

    // if this call failed, at most it counts as a read attempt that would have
    // been handled during `syscall_middle`
    if (ret < 0)
      return 0;

    // retrieve the FD
    int fd = peek_reg(tracee->pid, REG(rdi));

    // retrieve the path
    char *path = NULL;
    int rc = peek_string(&path, tracee->pid, REG(rsi));
    if (ERROR(rc != 0))
      return rc;

    // retrieve the flags
    int flags = peek_reg(tracee->pid, REG(rdx));

    if (fd == AT_FDCWD) {
      DEBUG("PID %d called openat(AT_FDCWD, \"%s\", %s | %d), ret %ld",
            (int)tracee->pid, path, flag_name(flags), other_flags(flags), ret);
    } else {
      DEBUG("PID %d called openat(%d, \"%s\", %s | %d), ret %ld",
            (int)tracee->pid, fd, path, flag_name(flags), other_flags(flags),
            ret);
    }

    // does this count as a write intent?
    bool is_write =
        (flags & O_RDWR) == O_RDWR || (flags & O_WRONLY) == O_WRONLY;
    if (is_write) {
      rc = see_write(tracee, fd, path);
      if (ERROR(rc != 0)) {
        free(path);
        return rc;
      }
    }

    // note the new file handle we need to learn
    rc = see_open(tracee, ret, fd, path);
    free(path);
    if (ERROR(rc != 0))
      return rc;

    return rc;
  }

  default:
    DEBUG("unrecognised syscall %ld", nr);
  }

  return 0;
}
