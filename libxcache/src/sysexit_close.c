#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <stddef.h>
#include <xcache/record.h>

int sysexit_close(proc_t *proc) {

  assert(proc != NULL);

  int rc = 0;

  // extract the file descriptor
  const long fd = peek_reg(proc->pid, REG(rdi));

  // extract the result
  const int err = peek_errno(proc->pid);

  DEBUG("pid %ld, close(%ld) = %d, errno == %d", (long)proc->pid, fd,
        err == 0 ? 0 : -1, err);

  // if it succeede, drop this from our tracking table
  if (err == 0) {
    if (ERROR(fd < 0 || (size_t)fd >= proc->n_fds || proc->fds[fd] == NULL)) {
      // the child somehow successfully closed something they did not have open
      rc = ECHILD;
      goto done;
    }

    fd_free(proc->fds[fd]);
    proc->fds[fd] = NULL;
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
  return rc;
}
