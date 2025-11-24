#include "debug.h"
#include "fd.h"
#include "inferior_t.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

/// handle `F_DUPFD`
///
/// @param inf Inferior that called `fcntl`
/// @param thread Thread that called `fcntl`
/// @param fd File descriptor `fcntl` was called on
/// @param cloexec Was this `F_DUPFD_CLOEXEC`?
/// @return 0 on success or an errno on failure
static int dupfd(inferior_t *inf, thread_t *thread, int fd, bool cloexec) {
  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  const long copy = peek_ret(thread);

  // if the operation failed, we can ignore it
  if (copy < 0)
    goto done;

  assert(copy <= INT_MAX && "kernel returned invalid file descriptor");
  assert(
      fd_at(thread->fd, fd) != NULL &&
      "child duplicated a file descriptor we believed they did not have open");
  assert(
      fd_at(thread->fd, (int)copy) == NULL &&
      "kernel duplicated into a file descriptor we believed was already open");

  // model the duplication
  const fd_t *const src = fd_at(thread->fd, fd);
  if (ERROR((rc = fd_open(thread->fd, (int)copy, src->path))))
    goto done;
  // TODO: does `F_DUPFD` of a cloexec descriptor copy cloexec?
  fd_at(thread->fd, (int)copy)->close_on_exec = cloexec;

done:
  return rc;
}

/// handle `F_SETFD`
///
/// @param inf Inferior that called `fcntl`
/// @param thread Thread that called `fcntl`
/// @param fd File descriptor `fcntl` was called on
/// @return 0 on success or an errno on failure
static int setfd(inferior_t *inf, thread_t *thread, int fd) {
  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  // if the operation failed, we can ignore it
  const long ret = peek_ret(thread);
  if (ret < 0)
    goto done;

  fd_t *const f = fd_at(thread->fd, fd);
  assert(f != NULL && "child `F_SETFD`-ed a file descriptor we believed they "
                      "did not have open");

  const long arg = peek_syscall_arg(thread, 3);
  f->close_on_exec = !!(arg & O_CLOEXEC);

done:
  return rc;
}

int sysexit_fcntl(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_syscall_arg(thread, 1);

  // extract the operation
  const int op = (int)peek_syscall_arg(thread, 2);

  DEBUG("TID %ld, fcntl(%d, %d, …)", (long)thread->id, fd, op);

  switch (op) {

  case F_DUPFD:
    if (ERROR((rc = dupfd(inf, thread, fd, false))))
      goto done;
    break;

  case F_DUPFD_CLOEXEC:
    if (ERROR((rc = dupfd(inf, thread, fd, true))))
      goto done;
    break;

  case F_SETFD:
    if (ERROR((rc = setfd(inf, thread, fd))))
      goto done;
    break;

  // none of the other operations are relevant to us
  default:
    break;
  }

done:
  return rc;
}
