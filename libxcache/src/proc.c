#include "proc.h"
#include "debug.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>

/// deallocate a process
static void proc_free(proc_t *proc) { free(proc); }

proc_t *proc_new(pid_t pid) {

  proc_t *p = NULL;
  proc_t *ret = NULL;

  p = calloc(1, sizeof(*p));
  if (ERROR(p == NULL))
    goto done;

  p->id = pid;
  p->ref_count = 1;

  ret = p;
  p = NULL;

done:
  proc_free(p);

  return ret;
}

proc_t *proc_acquire(proc_t *proc) {
  assert(proc != NULL);

  ++proc->ref_count;

  return proc;
}

proc_t *proc_release(proc_t *proc) {

  if (proc == NULL)
    return NULL;

  assert(proc->ref_count > 0 && "corrupted process reference counting");
  --proc->ref_count;

  if (proc->ref_count == 0)
    proc_free(proc);

  return NULL;
}
