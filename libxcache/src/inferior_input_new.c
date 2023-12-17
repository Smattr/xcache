#include "debug.h"
#include "inferior_t.h"
#include "input_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int inferior_input_new(inferior_t *inf, const input_t input) {

  assert(inf != NULL);

  // If this is already considered an output, ignore it. This can happen when
  // e.g. the target writes out a temporary file and then reads it back in.
  for (size_t i = 0; i < inf->n_outputs; ++i) {
    if (strcmp(inf->outputs[i].path, input.path) == 0) {
      DEBUG("skipping \"%s\" as input because it is already an output",
            input.path);
      return 0;
    }
  }

  // do we need to expand the input actions list?
  if (inf->n_inputs == inf->c_inputs) {
    const size_t c = inf->c_inputs == 0 ? 1 : inf->c_inputs * 2;
    input_t *a = realloc(inf->inputs, c * sizeof(inf->inputs[0]));
    if (ERROR(a == NULL))
      return ENOMEM;
    inf->c_inputs = c;
    inf->inputs = a;
  }

  assert(inf->n_inputs < inf->c_inputs);
  inf->inputs[inf->n_inputs] = input;
  ++inf->n_inputs;

  return 0;
}
