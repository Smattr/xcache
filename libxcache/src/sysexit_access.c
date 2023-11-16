#include "action_t.h"
#include "debug.h"
#include "path.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcache/record.h>

int sysexit_access(proc_t *proc) {

  assert(proc != NULL);

  char *path = NULL;
  char *abs = NULL;
  action_t saw = {0};
  int rc = 0;

  // extract the path
  const uintptr_t path_ptr = (uintptr_t)peek_reg(proc->pid, REG(rdi));
  if (ERROR((rc = peek_str(&path, proc->pid, path_ptr)))) {
    // if the read faulted, assume our side was correct and the tracee used a
    // bad pointer, something we do not support recording
    if (rc == EFAULT)
      rc = ECHILD;
    goto done;
  }

  // make it absolute
  abs = path_absolute(proc->cwd, path);
  if (ERROR(abs == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // extract the flags
  const long flags = peek_reg(proc->pid, REG(rsi));

  // treat any flag we do not know as the child doing something unsupported
  if (ERROR(flags & ~(R_OK | W_OK | X_OK | F_OK))) {
    rc = ECHILD;
    goto done;
  }

  // extract the result
  const int err = peek_errno(proc->pid);

  DEBUG("pid %ld, access(\"%s\", %ld) = %d, errno == %d", (long)proc->pid, path,
        flags, err == 0 ? 0 : -1, err);

  // record it
  if (ERROR((rc = action_new_access(&saw, err, abs, (int)flags))))
    goto done;

  if (ERROR((rc = proc_action_new(proc, saw))))
    goto done;
  saw = (action_t){0};

  // restart the process
  if (proc->mode == XC_SYSCALL) {
    if (ERROR((rc = proc_syscall(*proc))))
      goto done;
  } else {
    if (ERROR((rc = proc_cont(*proc))))
      goto done;
  }

done:
  action_free(saw);
  free(abs);
  free(path);

  return rc;
}
