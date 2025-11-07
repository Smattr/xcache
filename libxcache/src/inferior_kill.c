#include "inferior_t.h"
#include "list.h"
#include "proc_t.h"
#include <assert.h>
#include <signal.h>
#include <stddef.h>

void inferior_kill(inferior_t *inf) {

  assert(inf != NULL);

  for (size_t i = 0; i < LIST_SIZE(&inf->procs); ++i)
    proc_end(LIST_AT(&inf->procs, i));
}
