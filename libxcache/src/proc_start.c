#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcache/cmd.h>

int proc_start(proc_t *proc, const xc_cmd_t cmd) {

  assert(proc != NULL);
  assert(proc->pid == 0 && "proc already started?");

  int rc = 0;

  {
    pid_t pid = fork();
    if (ERROR(pid < 0)) {
      rc = errno;
      goto done;
    }

    if (pid == 0) {
      proc_exec(proc, cmd);
      // unreachable
    }

    proc->pid = pid;
  }

  DEBUG("waiting for the child (pid %ld) to SIGSTOP itself…", (long)proc->pid);
  {
    int status;
    if (ERROR(waitpid(proc->pid, &status, 0) < 0)) {
      rc = errno;
      goto done;
    }
    assert((WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(status) ||
            WIFCONTINUED(status)) &&
           "unknown waitpid status");
    assert(!WIFCONTINUED(status) &&
           "waitpid indicated SIGCONT when we did not request it");
    // if the child failed before it could SIGSTOP itself, it will have exited
    // with an errno as its status
    if (ERROR(WIFEXITED(status))) {
      rc = WEXITSTATUS(status);
      proc->pid = 0;
      goto done;
    }
    if (ERROR(WIFSIGNALED(status))) {
      DEBUG("child died with signal %d", WTERMSIG(status));
      errno = ECHILD;
      proc->pid = 0;
      goto done;
    }
    assert(WIFSTOPPED(status));
    if (ERROR(WSTOPSIG(status) != SIGSTOP)) {
      DEBUG("child stopped with signal %d", WSTOPSIG(status));
      rc = ECHILD;
      goto done;
    }
  }

  // set our tracer preferences
  {
    // TODO: support Linux < 3.5, missing PTRACE_O_SECCOMP
    const int opts = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACESECCOMP |
                     PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK |
                     PTRACE_O_TRACEVFORK;
    if (ERROR(ptrace(PTRACE_SETOPTIONS, proc->pid, NULL, opts) < 0)) {
      rc = errno;
      goto done;
    }
  }

  // resume the child
  // TODO: Do some more nuanced discrimination here:
  //   1. continue to next seccomp stop on Linux ≥ 3.5
  //   2. continue to next syscall on Linux < 3.5
  // I don’t think we care about the </≥4.8 differences
  if (ERROR((rc = proc_cont(*proc))))
    goto done;

done:
  return rc;
}
