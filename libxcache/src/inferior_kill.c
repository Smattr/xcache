#include "inferior_t.h"
#include <assert.h>
#include <stddef.h>
#include <signal.h>

void inferior_kill(inferior_t *inf) {

  assert(inf != NULL);

  for (size_t i = 0; i< inf->n_procs; ++i)
    (void)kill(inf->procs[i].id, SIGKILL);
}

