#include "debug.h"
#include "peek.h"
#include "tracee.h"
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

int syscall_middle(tracee_t *tracee) {

  assert(tracee != NULL);

  int rc = 0;

  // retrieve the syscall the child is midway through
  long nr = peek_syscall_no(tracee->pid);

  switch (nr) {

  // Handle a read `open` here because we do not care whether it succeeds or
  // fails. Either counts as a read attempt
  case __NR_open: {

    // retrieve the flags first to see if we need to continue
    int flags = peek_reg(tracee->pid, REG(rsi));
    bool is_read = (flags & O_RDWR) == O_RDWR || (flags & O_WRONLY) != O_WRONLY;

    if (is_read) {
      // retrieve the path
      char *path = NULL;
      rc = peek_string(&path, tracee->pid, REG(rdi));
      // FIXME: what happens if the tracee made a bad open call?
      if (ERROR(rc != 0))
        goto done;

      DEBUG("PID %d called open(\"%s\", …)", (int)tracee->pid, path);

      rc = see_read(tracee, AT_FDCWD, path);
      free(path);
      if (ERROR(rc != 0))
        goto done;
    }

    // resume the tracee, but we need to again intercept at syscall end to pick
    // up the newly allocated FD if the syscall was successful
    rc = tracee_resume_to_syscall(tracee);
    if (ERROR(rc != 0))
      goto done;

    break;
  }

  // `access` can be handled either here or on syscall exit, so we may as well
  // handle it here for efficiency
  case __NR_access: {

    // retrieve the path
    char *path = NULL;
    rc = peek_string(&path, tracee->pid, REG(rdi));
    if (ERROR(rc != 0))
      goto done;

    DEBUG("PID %d called access(\"%s\")", (int)tracee->pid, path);

    rc = see_read(tracee, AT_FDCWD, path);
    free(path);
    if (ERROR(rc != 0))
      goto done;

    rc = tracee_resume(tracee);
    if (ERROR(rc != 0))
      goto done;

    break;
  }

  // We need to handle `execve` here rather than at syscall exit because at
  // syscall exit the callee’s address space is gone, making it impossible to
  // read the (string) syscall argument. Conveniently, our handling of `execve`
  // does not depend on the syscall result.
  case __NR_execve: {

    // retrieve the path
    char *path = NULL;
    rc = peek_string(&path, tracee->pid, REG(rdi));
    if (ERROR(rc != 0))
      goto done;

    DEBUG("PID %d called execve(\"%s\")", (int)tracee->pid, path);

    rc = see_read(tracee, AT_FDCWD, path);
    free(path);
    if (ERROR(rc != 0))
      goto done;

    rc = tracee_resume(tracee);
    if (ERROR(rc != 0))
      goto done;

    break;
  }

  // treat a `chdir` attempt as a read
  case __NR_chdir: {

    // retrieve the path
    char *path = NULL;
    rc = peek_string(&path, tracee->pid, REG(rdi));
    if (ERROR(rc != 0))
      goto done;

    DEBUG("PID %d called chdir(\"%s\")", (int)tracee->pid, path);

#if 0 // TODO
    rc = see_read(tracee, AT_FDCWD, path);
#endif
    free(path);
    if (ERROR(rc != 0))
      goto done;

    // we need to see the other side of `chdir` to potentially update
    // `tracee->cwd`
    rc = tracee_resume_to_syscall(tracee);
    if (ERROR(rc != 0))
      goto done;

    break;
  }

  // Handle a read `openat` here because we do not care whether it succeeds or
  // fails. Either counts as a read attempt.
  case __NR_openat: {

    // retrieve the flags first to see if we need to continue
    int flags = peek_reg(tracee->pid, REG(rdx));
    bool is_read = (flags & O_RDWR) == O_RDWR || (flags & O_WRONLY) != O_WRONLY;

    if (is_read) {
      // retrieve the FD
      int fd = peek_reg(tracee->pid, REG(rdi));

      // retrieve the path
      char *path = NULL;
      rc = peek_string(&path, tracee->pid, REG(rsi));
      // FIXME: what happens if the tracee made a bad openat call?
      if (ERROR(rc != 0))
        goto done;

      if (fd == AT_FDCWD) {
        DEBUG("PID %d called openat(AT_FDCWD, \"%s\", …)", (int)tracee->pid,
              path);
      } else {
        DEBUG("PID %d called openat(%d, \"%s\", …)", (int)tracee->pid, fd,
              path);
      }

      // TODO: handle the tracee passing an invalid fd

      rc = see_read(tracee, fd, path);
      free(path);
      if (ERROR(rc != 0))
        goto done;
    }

    // resume the tracee, but we need to again intercept at syscall end to pick
    // up the newly allocated FD if the syscall was successful
    rc = tracee_resume_to_syscall(tracee);
    if (ERROR(rc != 0))
      goto done;

    break;
  }

  // newfstatat counts as a read
  case __NR_newfstatat: {

    // retrieve the FD
    int fd = peek_reg(tracee->pid, REG(rdi));

    // retrieve the path
    char *path = NULL;
    rc = peek_string(&path, tracee->pid, REG(rsi));
    if (ERROR(rc != 0))
      goto done;

    if (fd == AT_FDCWD) {
      DEBUG("PID %d called newfstatat(AT_FDCWD, \"%s\", …)", (int)tracee->pid,
            path);
    } else {
      DEBUG("PID %d called newfstatat(%d, \"%s\", …)", (int)tracee->pid, fd,
            path);
    }

    rc = see_read(tracee, fd, path);
    free(path);
    if (ERROR(rc != 0))
      goto done;

    rc = tracee_resume(tracee);
    if (ERROR(rc != 0))
      goto done;

    break;
  }

  // any other syscalls we either do not care about or handle at syscall exit
  default:
    DEBUG("ignored seccomp stop for syscall %ld", nr);
    rc = tracee_resume_to_syscall(tracee);
    if (ERROR(rc != 0))
      goto done;

    break;
  }

done:
  return rc;
}
