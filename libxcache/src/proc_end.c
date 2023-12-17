#include "proc_t.h"
#include <assert.h>
#include <signal.h>
#include <stdlib.h>

void proc_end(proc_t *proc) {

  assert(proc != NULL);

  // the process may have already terminated (or never started)
  if (proc->id != 0) {
    (void)kill(proc->id, SIGKILL);
    // TODO: wait() on the process
  }
  proc->id = 0;

  free(proc->threads);
  proc->threads = NULL;
  proc->n_threads = 0;
  proc->c_threads = 0;

  proc_fds_free(proc);

  free(proc->cwd);
  proc->cwd = NULL;
}
