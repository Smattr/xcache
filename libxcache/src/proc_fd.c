#include "thread_t.h"
#include <assert.h>
#include <stddef.h>

const fd_t *proc_fd(const proc_t *proc, int fd) {

  assert(proc != NULL);

  if (fd < 0)
    return NULL;

  if ((size_t)fd >= proc->n_fds)
    return NULL;

  return proc->fds[fd];
}
