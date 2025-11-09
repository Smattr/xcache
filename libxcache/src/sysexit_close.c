#include "../../common/proccall.h"
#include "debug.h"
#include "inferior_t.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <stddef.h>
#include <xcache/record.h>

int sysexit_close(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
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
    if (ERROR(proc_fd(thread->proc, fd) == NULL)) {
      // the child somehow successfully closed something they did not have open
      rc = ECHILD;
      goto done;
    }

    fd_free(thread->proc->fds[fd]);
    thread->proc->fds[fd] = NULL;
  }

done:
  return rc;
}
