#include "debug.h"
#include "list.h"
#include "thread_t.h"
#include <assert.h>
#include <stddef.h>

int proc_fd_new(proc_t *proc, int fd, const char *path) {

  assert(proc != NULL);
  assert(fd >= 0);
  assert(path != NULL);

  int rc = 0;

  // do we need to enlarge the file descriptor table?
  while ((size_t)fd >= LIST_SIZE(&proc->fds)) {
    if (ERROR((rc = LIST_PUSH_BACK(&proc->fds, NULL))))
      goto done;
  }

  // close any previous entry
  fd_free(*LIST_AT(&proc->fds, (size_t)fd));
  *LIST_AT(&proc->fds, (size_t)fd) = NULL;

  if (ERROR((rc = fd_new(LIST_AT(&proc->fds, (size_t)fd), path))))
    goto done;

done:
  return rc;
}
