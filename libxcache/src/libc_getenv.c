#include "debug.h"
#include "inferior_t.h"
#include "input_t.h"
#include "peek.h"
#include "set.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/// handle the tracee having signalled us with `CALL_GETENV`
int libc_getenv(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  char *name = NULL;
  input_t input = {0};
  int rc = 0;

  // retrieve the environment variable being looked up
  const uintptr_t arg = (uintptr_t)peek_syscall_arg(thread, 3);
  if (arg == 0) {
    // tracee called `getenv(NULL)`; give up
    DEBUG("TID %ld called getenv(NULL)", (long)thread->id);
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

  // if this `getenv` is reading a variable that was previously set (as opposed
  // to something externally provided), ignore it
  if (set_contains(&thread->proc->env, name)) {
    DEBUG("TID %ld called getenv(\"%s\") but previously set $%s, so ignoring",
          (long)thread->id, name, name);
    goto done;
  }

  // Figure out what value the tracee saw. We could equally well have the spy
  // tell us the return value of `getenv`, but our tracking of the tracee’s
  // environment(s) is intended to be fully accurate, so this should be
  // equivalent.
  const char *const value = getenv(name);

  DEBUG("TID %ld called getenv(\"%s\") == %s%s%s", (long)thread->id, name,
        value == NULL ? "" : "\"", value == NULL ? "NULL" : value,
        value == NULL ? "" : "\"");

  if (ERROR((rc = input_new_getenv(&input, name, value))))
    goto done;
  if (ERROR((rc = inferior_input_new(inf, input))))
    goto done;
  input = (input_t){0};

done:
  input_free(input);
  free(name);

  return rc;
}
