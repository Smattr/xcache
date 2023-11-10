#include "debug.h"
#include "path.h"
#include "peek.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

int sysexit_chdir(proc_t *proc) {

  assert(proc != NULL);

  char *path = NULL;
  char *abs = NULL;
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

  // extract the result
  const long ret = peek_reg(proc->pid, REG(rax));

  DEBUG("pid %ld, chdir(\"%s\") = %ld", (long)proc->pid, path, ret);

  // if it succeeded, update our cwd
  if (ret == 0) {
    free(proc->cwd);
    proc->cwd = abs;
    abs = NULL;
  }

  // TODO: track the read attempt
  rc = ENOTSUP;

done:
  free(abs);
  free(path);

  return rc;
}
