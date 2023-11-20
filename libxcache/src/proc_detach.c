#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <stddef.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

void proc_detach(proc_t *proc) {

  assert(proc != 0);

  // has the child already terminated (or never started)?
  if (proc->pid == 0)
    return;

  if (ERROR(ptrace(PTRACE_DETACH, proc->pid, NULL, NULL) < 0)) {
    // ignore
  }

  if (ERROR(waitpid(proc->pid, NULL, 0) < 0)) {
    // ignore
  }

  proc->pid = 0;

  // cleanup child state in proc_t
  proc_end(proc);
}
