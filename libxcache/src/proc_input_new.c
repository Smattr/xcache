#include "debug.h"
#include "input_t.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int proc_input_new(proc_t *proc, const input_t input) {

  assert(proc != NULL);

  // If this is already considered an output, ignore it. This can happen when
  // e.g. the child writes out a temporary file and then reads it back in.
  for (size_t i = 0; i < proc->n_outputs; ++i) {
    if (strcmp(proc->outputs[i].path, input.path) == 0) {
      DEBUG("skipping \"%s\" as input because it is already an output",
            input.path);
      return 0;
    }
  }

  // do we need to expand the input actions list?
  if (proc->n_inputs == proc->c_inputs) {
    const size_t c = proc->c_inputs == 0 ? 1 : proc->c_inputs * 2;
    input_t *a = realloc(proc->inputs, c * sizeof(proc->inputs[0]));
    if (ERROR(a == NULL))
      return ENOMEM;
    proc->c_inputs = c;
    proc->inputs = a;
  }

  assert(proc->n_inputs < proc->c_inputs);
  proc->inputs[proc->n_inputs] = input;
  ++proc->n_inputs;

  return 0;
}
