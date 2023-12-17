#include "debug.h"
#include "inferior_t.h"
#include "output_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

int inferior_output_new(inferior_t *inf, const output_t output) {

  assert(inf != NULL);

  // do we need to expand the output actions list?
  if (inf->n_outputs == inf->c_outputs) {
    const size_t c = inf->c_outputs == 0 ? 1 : inf->c_outputs * 2;
    output_t *a = realloc(inf->outputs, c * sizeof(inf->outputs[0]));
    if (ERROR(a == NULL))
      return ENOMEM;
    inf->c_outputs = c;
    inf->outputs = a;
  }

  assert(inf->n_outputs < inf->c_outputs);
  inf->outputs[inf->n_outputs] = output;
  ++inf->n_outputs;

  return 0;
}
