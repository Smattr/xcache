#include "debug.h"
#include "inferior_t.h"
#include "peek.h"
#include "set.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

int libc_setenv(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  (void)inf;

  char *name = NULL;
  int rc = 0;

  // retrieve the environment variable being set
  const uintptr_t arg = (uintptr_t)peek_syscall_arg(thread, 3);
  if (arg == 0) {
    // give up if the tracee passed `NULL` to `setenv`
    DEBUG("TID %ld called setenv(NULL, …)", (long)thread->id);
    rc = ECHILD;
    goto done;
  }
  if (ERROR((rc = peek_str(&name, thread->proc, arg)))) {
    // if the read faulted, assume our side was correct and the spy used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  DEBUG("TID %ld called setenv(\"%s\", …)", (long)thread->id, name);

  if (ERROR((rc = set_add(&thread->proc->env, name))))
    goto done;

done:
  free(name);

  return rc;
}
