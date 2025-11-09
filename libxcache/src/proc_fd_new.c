#include "debug.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int proc_fd_new(proc_t *proc, int fd, const char *path) {

  assert(proc != NULL);
  assert(fd >= 0);
  assert(path != NULL);

  int rc = 0;

  // do we need to enlarge the file descriptor table?
  if ((size_t)fd >= proc->n_fds) {
    size_t n_fds = (size_t)fd + 1;
    fd_t **fds = realloc(proc->fds, n_fds * sizeof(*fds));
    if (ERROR(fds == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    memset(&fds[proc->n_fds], 0, (n_fds - proc->n_fds) * sizeof(proc->fds[0]));
    proc->fds = fds;
    proc->n_fds = n_fds;
  }

  // close any previous entry
  fd_free(proc->fds[fd]);
  proc->fds[fd] = NULL;

  if (ERROR((rc = fd_new(&proc->fds[fd], path))))
    goto done;

done:
  return rc;
}
