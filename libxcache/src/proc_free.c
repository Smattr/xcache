#include "proc_t.h"
#include <unistd.h>

void proc_free(proc_t proc) {

  if (proc.errfd[0] > 0)
    (void)close(proc.errfd[0]);
  if (proc.errfd[1] > 0)
    (void)close(proc.errfd[1]);

  if (proc.outfd[0] > 0)
    (void)close(proc.outfd[0]);
  if (proc.outfd[1] > 1)
    (void)close(proc.outfd[1]);
}