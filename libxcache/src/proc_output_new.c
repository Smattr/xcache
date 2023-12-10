#include "debug.h"
#include "output_t.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

int proc_output_new(proc_t *proc, const output_t output) {

  assert(proc != NULL);

  // do we need to expand the output actions list?
  if (proc->n_outputs == proc->c_outputs) {
    const size_t c = proc->c_outputs == 0 ? 1 : proc->c_outputs * 2;
    output_t *a = realloc(proc->outputs, c * sizeof(proc->outputs[0]));
    if (ERROR(a == NULL))
      return ENOMEM;
    proc->c_outputs = c;
    proc->outputs = a;
  }

  assert(proc->n_outputs < proc->c_outputs);
  proc->outputs[proc->n_outputs] = output;
  ++proc->n_outputs;

  return 0;
}
