#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/version.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcache/cmd.h>

int proc_start(proc_t *proc, const xc_cmd_t cmd) {

  assert(proc != NULL);
  assert(proc->pid == 0 && "proc already started?");

  int rc = 0;

  free(proc->cwd);
  proc->cwd = strdup(cmd.cwd);
  if (ERROR(proc->cwd == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  proc_fds_free(proc);
  // we dup /dev/null over the child’s stdin
  if (ERROR((rc = proc_fd_new(proc, STDIN_FILENO, "/dev/null"))))
    goto done;
  if (ERROR((rc = proc_fd_new(proc, STDOUT_FILENO, "/dev/stdout"))))
    goto done;
  if (ERROR((rc = proc_fd_new(proc, STDERR_FILENO, "/dev/stderr"))))
    goto done;

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
    int opts = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE |
               PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK;
    // We do not care about `PTRACE_EVENT_EXEC`, but we set `PTRACE_O_TRACEEXEC`
    // anyway. This seems the only way to avoid a `SIGTRAP` after `execve`. This
    // exec `SIGTRAP` is indistinguishable from a regular `SIGTRAP`, so we would
    // otherwise have to do our own tracking of which PID(s) had just called
    // `execve` and would thus receive one of these.
    opts |= PTRACE_O_TRACEEXEC;
#if LINUX_VERSION_MAJOR > 3 ||                                                 \
    (LINUX_VERSION_MAJOR == 3 && LINUX_VERSION_MINOR > 4)
    if (proc->mode == XC_EARLY_SECCOMP || proc->mode == XC_LATE_SECCOMP)
      opts |= PTRACE_O_TRACESECCOMP;
#endif
    if (ERROR(ptrace(PTRACE_SETOPTIONS, proc->pid, NULL, opts) < 0)) {
      rc = errno;
      goto done;
    }
  }

  // resume the child
  if (proc->mode == XC_SYSCALL) {
    if (ERROR((rc = proc_syscall(*proc))))
      goto done;
  } else {
    if (ERROR((rc = proc_cont(*proc))))
      goto done;
  }

done:
  return rc;
}
