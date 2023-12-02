#include "input_t.h"
#include "proc_t.h"
#include <stdlib.h>
#include <unistd.h>

void proc_free(proc_t proc) {

  proc_end(&proc);

  for (size_t i = 0; i < proc.n_inputs; ++i)
    input_free(proc.inputs[i]);
  free(proc.inputs);

  if (proc.errfd[0] > 0)
    (void)close(proc.errfd[0]);
  if (proc.errfd[1] > 0)
    (void)close(proc.errfd[1]);

  if (proc.outfd[0] > 0)
    (void)close(proc.outfd[0]);
  if (proc.outfd[1] > 0)
    (void)close(proc.outfd[1]);
}
