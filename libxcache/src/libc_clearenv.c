#include "debug.h"
#include "inferior_t.h"
#include "peek.h"
#include "set.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <stddef.h>

int libc_clearenv(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  // retrieve the payload of this call
  const int ret = (int)peek_syscall_arg(thread, 3);

  DEBUG("TID %ld called clearenv() == %d", (long)thread->id, ret);

  // if this succeeded, we do not consider future `getenv`s as consuming
  // external information
  if (ret == 0) {
    if (ERROR((rc = set_add_universe(&thread->proc->env))))
      goto done;
  }

done:
  return rc;
}
