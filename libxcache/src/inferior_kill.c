#include "inferior_t.h"
#include "proc_t.h"
#include <assert.h>
#include <signal.h>
#include <stddef.h>

void inferior_kill(inferior_t *inf) {

  assert(inf != NULL);

  for (size_t i = 0; i < inf->n_procs; ++i)
    proc_end(&inf->procs[i]);
}
