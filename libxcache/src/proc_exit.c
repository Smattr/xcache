#include "proc_t.h"
#include <assert.h>
#include <stdlib.h>

void proc_exit(proc_t *proc, int exit_status) {

  assert(proc != NULL);

  proc_fds_free(proc);

  proc->exit_status = exit_status;

  free(proc->threads);
  proc->threads = NULL;
  proc->n_threads = 0;
  proc->c_threads = 0;

  proc->id = 0;

  free(proc->cwd);
  proc->cwd = NULL;
}
