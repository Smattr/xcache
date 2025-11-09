#include "list.h"
#include "thread_t.h"
#include <assert.h>
#include <stddef.h>

void proc_fds_free(proc_t *proc) {

  assert(proc != NULL);

  for (size_t i = 0; i < LIST_SIZE(&proc->fds); ++i)
    fd_free(*LIST_AT(&proc->fds, i));
  LIST_FREE(&proc->fds);
}
