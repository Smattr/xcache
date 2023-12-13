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

  {
    int status = 0;
    if (ERROR(waitpid(proc->pid, &status, 0) < 0)) {
      goto done;
    }

    assert(!WIFSTOPPED(status) && "saw child stop while untrace");
    if (WIFEXITED(status)) {
      proc->exit_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      // replicate Linux conventions
      proc->exit_status = 128 + WTERMSIG(status);
    }
  }

  proc->pid = 0;

done:
  // cleanup child state in proc_t
  proc_end(proc);
}
