#include "macros.h"
#include "proc.h"
#include <errno.h>
#include <unistd.h>
#include <xcache/proc.h>

int xc_proc_exec(const xc_proc_t *proc) {

  if (UNLIKELY(proc == NULL))
    return EINVAL;

  int rc = chdir(proc->cwd);
  if (UNLIKELY(rc != 0))
    return errno;

  rc = execvp(proc->argv[0], proc->argv);
  if (UNLIKELY(rc != 0))
    return errno;

  UNREACHABLE();
}
