#include "inferior_t.h"
#include "input_t.h"
#include "list.h"
#include "output_t.h"
#include "tee_t.h"
#include "thread_t.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

void inferior_free(inferior_t *inf) {

  assert(inf != NULL);

  inferior_kill(inf);

  assert(LIST_SIZE(&inf->threads) == 0);
  LIST_FREE(&inf->threads, NULL);

  LIST_FREE(&inf->outputs, output_free);
  LIST_FREE(&inf->inputs, input_free);

  if (inf->exec_status[0] > 0)
    (void)close(inf->exec_status[0]);
  if (inf->exec_status[1] > 0)
    (void)close(inf->exec_status[1]);

  tee_cancel(inf->t_err);
  tee_cancel(inf->t_out);

  *inf = (inferior_t){0};
}
