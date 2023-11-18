#include "debug.h"
#include "input_t.h"
#include "path.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <xcache/record.h>

int sysexit_openat(proc_t *proc) {

  assert(proc != NULL);

  char *path = NULL;
  char *abs = NULL;
  input_t saw = {0};
  int rc = 0;

  // extract the file descriptor
  const long fd = peek_reg(proc->pid, REG(rdi));

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_reg(proc->pid, REG(rsi));
  if (ERROR((rc = peek_str(&path, proc->pid, path_ptr)))) {
    // if the read faulted, assume our side was correct and the tracee used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  // extract the flags
  const long flags = peek_reg(proc->pid, REG(rdx));

  // extract the result
  const int err = peek_errno(proc->pid);

  if (UNLIKELY(xc_debug != NULL)) {
    char *fd_str = atfd_to_str(fd);
    char *flags_str = openflags_to_str(flags);
    DEBUG("pid %ld, openat(%s, \"%s\", %s, …) = %d, errno == %d",
          (long)proc->pid, fd_str == NULL ? "<oom>" : fd_str, path,
          flags_str == NULL ? "<oom>" : flags_str, err == 0 ? 0 : -1, err);
    free(flags_str);
    free(fd_str);
  }

  // make the path absolute
  if (path[0] == '/') {
    // fd is ignored
    abs = path;
    path = NULL;
  } else if (fd == AT_FDCWD) {
    abs = path_absolute(proc->cwd, path);
    if (ERROR(abs == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  } else {
    // TODO
    rc = ENOTSUP;
    goto done;
  }

  // discard the flags that have no relevance to us
  const long flags_relevant =
      flags & ~(O_ASYNC | O_CLOEXEC | O_DIRECT | O_DSYNC | O_LARGEFILE |
                O_NOCTTY | O_NONBLOCK | O_NDELAY | O_SYNC);

  // TODO
  if (ERROR(flags_relevant != O_RDONLY)) {
    rc = ENOTSUP;
    goto done;
  }

  // record it
  if (ERROR((rc = input_new_read(&saw, err, abs))))
    goto done;

  if (ERROR((rc = proc_input_new(proc, saw))))
    goto done;
  saw = (input_t){0};

  // if it succeeded, update the file descriptor table
  if (err == 0) {
    const long ret = peek_ret(proc->pid);
    assert(ret >= 0 && "logic error");
    assert(ret <= INT_MAX && "unexpected kernel return from openat");
    DEBUG("pid %ld, updating FD %ld → \"%s\"", (long)proc->pid, ret, abs);
    if (ERROR((rc = proc_fd_new(proc, (int)ret, abs))))
      goto done;
  }

  // restart the process
  if (proc->mode == XC_SYSCALL) {
    if (ERROR((rc = proc_syscall(*proc))))
      goto done;
  } else {
    if (ERROR((rc = proc_cont(*proc))))
      goto done;
  }

done:
  input_free(saw);
  free(abs);
  free(path);

  return rc;
}