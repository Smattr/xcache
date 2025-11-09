#include "list.h"
#include "thread_t.h"
#include <assert.h>
#include <stddef.h>

void proc_fds_free(proc_t *proc) {

  assert(proc != NULL);

  LIST_FREE(&proc->fds, fd_free);
}
