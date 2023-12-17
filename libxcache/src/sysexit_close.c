#include "../../common/proccall.h"
#include "debug.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <stddef.h>
#include <xcache/record.h>
#include "inferior_t.h"

int sysexit_close(inferior_t *inf, proc_t *proc, thread_t *thread) {

assert(inf != NULL);
  assert(proc != NULL);
  assert(thread != NULL);

  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_reg(thread, REG(rdi));

  // extract the result
  const int err = peek_errno(thread);

  DEBUG("TID %ld, close(%d) = %d, errno == %d", (long)thread->id, fd,
        err == 0 ? 0 : -1, err);

  // if the child closed our spy’s channel back to us, consider this unsupported
  if (ERROR(fd == XCACHE_FILENO)) {
    rc = ECHILD;
    goto done;
  }

  // if it succeeded, drop this from our tracking table
  if (err == 0) {
    if (ERROR(fd < 0 || (size_t)fd >= proc->n_fds || proc->fds[fd] == NULL)) {
      // the child somehow successfully closed something they did not have open
      rc = ECHILD;
      goto done;
    }

    fd_free(proc->fds[fd]);
    proc->fds[fd] = NULL;
  }

done:
  return rc;
}
