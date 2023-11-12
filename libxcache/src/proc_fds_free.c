#include "proc_t.h"
#include <assert.h>
#include <stdlib.h>

void proc_fds_free(proc_t *proc) {

  assert(proc != NULL);

  for (size_t i = 0; i < proc->n_fds; ++i)
    fd_free(proc->fds[i]);
  free(proc->fds);
  proc->fds = NULL;
  proc->n_fds = 0;
}
