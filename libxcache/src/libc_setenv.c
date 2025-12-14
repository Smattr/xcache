#include "../../common/proccall.h"
#include "debug.h"
#include "inferior_t.h"
#include "input.h"
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

  char *name = NULL;
  input_t input = {0};
  int rc = 0;

  // retrieve the payload of this call
  const uintptr_t arg = (uintptr_t)peek_syscall_arg(thread, 3);
  assert(arg != 0);
  setenv_t payload;
  if (ERROR((rc = PEEK_OBJ(&payload, thread->proc, arg))))
    goto done;

  if (payload.name == 0) {
    // give up if the tracee passed `NULL` to `setenv`
    DEBUG("TID %ld called setenv(NULL, …)", (long)thread->id);
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

  DEBUG("TID %ld called setenv(\"%s\", …, %d) == %d", (long)thread->id, name,
        payload.overwrite, payload.ret);

  // without `overwrite`, this is an implied `getenv`
  if (!payload.overwrite) {
    if (!set_contains(&thread->proc->env, name)) {
      const char *const value = getenv(name);
      if (ERROR((rc = input_new_getenv(&input, name, value))))
        goto done;
      if (ERROR((rc = inferior_input_new(inf, input))))
        goto done;
      input = (input_t){0};
    }
  }

  // if this succeeded, we do not consider future `getenv`s as consuming
  // external information
  if (payload.ret == 0) {
    if (ERROR((rc = set_add(&thread->proc->env, name))))
      goto done;
  }

done:
  input_free(input);
  free(name);

  return rc;
}
