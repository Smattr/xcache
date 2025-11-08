#include "inferior_t.h"
#include "input_t.h"
#include "list.h"
#include "output_t.h"
#include "proc_t.h"
#include "tee_t.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

void inferior_free(inferior_t *inf) {

  assert(inf != NULL);

  inferior_kill(inf);

  LIST_FREE(&inf->procs);

  for (size_t i = 0; i < LIST_SIZE(&inf->outputs); ++i)
    output_free(*LIST_AT(&inf->outputs, i));
  LIST_FREE(&inf->outputs);

  for (size_t i = 0; i < LIST_SIZE(&inf->inputs); ++i)
    input_free(*LIST_AT(&inf->inputs, i));
  LIST_FREE(&inf->inputs);

  if (inf->exec_status[0] > 0)
    (void)close(inf->exec_status[0]);
  if (inf->exec_status[1] > 0)
    (void)close(inf->exec_status[1]);

  if (inf->proccall[0] > 0)
    (void)close(inf->proccall[0]);
  if (inf->proccall[1] > 0)
    (void)close(inf->proccall[1]);

  tee_cancel(inf->t_err);
  tee_cancel(inf->t_out);

  *inf = (inferior_t){0};
}
