#include "list.h"
#include "thread_t.h"
#include <assert.h>
#include <stddef.h>

const fd_t *proc_fd(const proc_t *proc, int fd) {

  assert(proc != NULL);

  if (fd < 0)
    return NULL;

  if ((size_t)fd >= LIST_SIZE(&proc->fds))
    return NULL;

  return *LIST_AT(&proc->fds, (size_t)fd);
}
