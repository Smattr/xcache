#include "input_t.h"
#include "proc_t.h"
#include "tee_t.h"
#include <stdlib.h>
#include <unistd.h>

void proc_free(proc_t proc) {

  proc_end(&proc);

  for (size_t i = 0; i < proc.n_inputs; ++i)
    input_free(proc.inputs[i]);
  free(proc.inputs);

  if (proc.proccall[0] > 0)
    (void)close(proc.proccall[0]);
  if (proc.proccall[1] > 0)
    (void)close(proc.proccall[1]);

  tee_cancel(proc.t_err);
  tee_cancel(proc.t_out);
}
