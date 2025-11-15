#include "thread_t.h"
#include <assert.h>
#include <stdlib.h>

void proc_free(proc_t *proc) {

  if (proc == NULL)
    return;

  assert(proc->reference_count > 0 && "corrupted process reference counting");
  --proc->reference_count;

  // if there are still remaining references to this process, we cannot discard
  // it
  if (proc->reference_count > 0)
    return;

  proc_fds_free(proc);

  free(proc);
}
