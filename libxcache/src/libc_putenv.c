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
#include <string.h>

int libc_putenv(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  char *string = NULL;
  int rc = 0;

  // retrieve the payload of this call
  const uintptr_t arg = (uintptr_t)peek_syscall_arg(thread, 3);
  assert(arg != 0);
  putenv_t payload;
  if (ERROR((rc = PEEK_OBJ(&payload, thread->proc, arg))))
    goto done;

  if (payload.string == 0) {
    // give up if the tracee passed `NULL` to `putenv`
    DEBUG("TID %ld called putenv(NULL, …)", (long)thread->id);
    rc = ECHILD;
    goto done;
  }
  // retrieve the `key=value` string
  if (ERROR((rc = peek_str(&string, thread->proc, payload.string)))) {
    // if the read faulted, assume our side was correct and the spy used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  DEBUG("TID %ld called putenv(\"%s\") == %d", (long)thread->id, string,
        payload.ret);

  // slice the name out, or give up if there was no '=' divider
  char *const equals = strchr(string, '=');
  if (ERROR(equals == NULL)) {
    rc = ECHILD;
    goto done;
  }
  *equals = '\0';

  // if this succeeded, we do not consider future `getenv`s as consuming
  // external information
  if (payload.ret == 0) {
    if (ERROR((rc = set_add(&thread->proc->env, string))))
      goto done;
  }

done:
  free(string);

  return rc;
}
