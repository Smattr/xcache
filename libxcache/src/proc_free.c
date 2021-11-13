#include "proc.h"
#include <stdlib.h>
#include <xcache/proc.h>

void xc_proc_free(xc_proc_t *proc) {

  if (proc == NULL)
    return;

  free(proc->cwd);

  for (size_t i = 0; i < proc->argc; ++i)
    free(proc->argv[i]);
  free(proc->argv);

  free(proc);
}
