#include "action_t.h"
#include "proc_t.h"
#include <unistd.h>

void proc_free(proc_t proc) {

  proc_end(&proc);

  action_free_all(proc.actions);

  proc_fds_free(&proc);

  if (proc.errfd[0] > 0)
    (void)close(proc.errfd[0]);
  if (proc.errfd[1] > 0)
    (void)close(proc.errfd[1]);

  if (proc.outfd[0] > 0)
    (void)close(proc.outfd[0]);
  if (proc.outfd[1] > 1)
    (void)close(proc.outfd[1]);
}
