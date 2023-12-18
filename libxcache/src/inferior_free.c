#include "inferior_t.h"
#include "input_t.h"
#include "output_t.h"
#include "proc_t.h"
#include "tee_t.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

void inferior_free(inferior_t *inf) {

  assert(inf != NULL);

  inferior_kill(inf);

  free(inf->procs);

  for (size_t i = 0; i < inf->n_outputs; ++i)
    output_free(inf->outputs[i]);
  free(inf->outputs);

  for (size_t i = 0; i < inf->n_inputs; ++i)
    input_free(inf->inputs[i]);
  free(inf->inputs);

  if (inf->proccall[0] > 0)
    (void)close(inf->proccall[0]);
  if (inf->proccall[1] > 0)
    (void)close(inf->proccall[1]);

  tee_cancel(inf->t_err);
  tee_cancel(inf->t_out);

  *inf = (inferior_t){0};
}
