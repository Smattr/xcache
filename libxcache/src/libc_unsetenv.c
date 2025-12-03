#include "../../common/proccall.h"
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

int libc_unsetenv(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  (void)inf;

  char *name = NULL;
  int rc = 0;

  // retrieve the payload of this call
  const uintptr_t arg = (uintptr_t)peek_syscall_arg(thread, 3);
  assert(arg != 0);
  unsetenv_t payload;
  if (ERROR((rc = PEEK_OBJ(&payload, thread->proc, arg))))
    goto done;

  if (payload.name == 0) {
    // give up if the tracee passed `NULL` to `unsetenv`
    DEBUG("TID %ld called unsetenv(NULL)", (long)thread->id);
    rc = ECHILD;
    goto done;
  }
  // retrieve the environment variable being set
  if (ERROR((rc = peek_str(&name, thread->proc, payload.name)))) {
    // if the read faulted, assume our side was correct and the spy used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  DEBUG("TID %ld called unsetenv(\"%s\") == %d", (long)thread->id, name,
        payload.ret);

  // if this succeeded, we do not consider future `getenv`s as consuming
  // external information
  if (payload.ret == 0) {
    if (ERROR((rc = set_add(&thread->proc->env, name))))
      goto done;
  }

done:
  free(name);

  return rc;
}
