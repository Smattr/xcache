#include "proc_t.h"
#include <assert.h>
#include <signal.h>
#include <stdlib.h>

void proc_end(proc_t *proc) {

  assert(proc != NULL);

  // the process may have already terminated (or never started)
  if (proc->pid != 0) {
    (void)kill(proc->pid, SIGKILL);
    // TODO: wait() on the process
  }
  proc->pid = 0;

  free(proc->cwd);
  proc->cwd = NULL;
}
