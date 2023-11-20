#include "debug.h"
#include "input_t.h"
#include "path.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int sysenter_execve(proc_t *proc) {

  assert(proc != NULL);

  char *path = NULL;
  char *abs = NULL;
  input_t saw = {0};
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

  DEBUG("pid %ld, execve(\"%s\", …)", (long)proc->pid, path);

  // infer a read() from execve()
  if (ERROR((rc = input_new_read(&saw, 0, abs))))
    goto done;
  if (ERROR((rc = proc_input_new(proc, saw))))
    goto done;
  saw = (input_t){0};

  // infer an access() with X_OK from execve()
  if (ERROR((rc = input_new_access(&saw, 0, abs, X_OK))))
    goto done;
  if (ERROR((rc = proc_input_new(proc, saw))))
    goto done;
  saw = (input_t){0};

  // restart the process
  if (proc->mode == XC_SYSCALL) {
    if (ERROR((rc = proc_syscall(*proc))))
      goto done;
  } else {
    if (ERROR((rc = proc_cont(*proc))))
      goto done;
  }

done:
  input_free(saw);
  free(abs);
  free(path);

  return rc;
}
