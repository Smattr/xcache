#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/wait.h>

void proc_end(proc_t *proc) {

  assert(proc != NULL);

  // the process may have already terminated (or never started)
  if (proc->id != 0) {
    do {
      if (ERROR(kill(proc->id, SIGKILL) < 0))
        break;
      for (size_t i = proc->n_threads - 1; i != SIZE_MAX; --i) {
        int ignored = 0;
        (void)waitpid(proc->threads[i].id, &ignored, __WALL | __WNOTHREAD);
        --proc->n_threads;
      }
    } while (0);
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
