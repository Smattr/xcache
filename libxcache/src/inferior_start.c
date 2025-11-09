#include "../../common/proccall.h"
#include "debug.h"
#include "find_me.h"
#include "inferior_t.h"
#include "list.h"
#include "thread_t.h"
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

int inferior_start(inferior_t *inf, const xc_cmd_t cmd) {

  assert(inf != NULL);
  assert(LIST_SIZE(&inf->threads) == 0 && "inferior already started?");

  char *spy = NULL;
  thread_t thread = {0};
  int rc = 0;

  if (ERROR((rc = find_spy(&spy))))
    goto done;

  {
    proc_t *const proc = calloc(1, sizeof(*proc));
    if (proc == NULL) {
      rc = ENOMEM;
      goto done;
    }

    thread.proc = proc;
    ++proc->reference_count;

    proc->cwd = strdup(cmd.cwd);
    if (ERROR(proc->cwd == NULL)) {
      rc = ENOMEM;
      goto done;
    }

    // we dup /dev/null over the child’s stdin
    if (ERROR((rc = proc_fd_new(proc, STDIN_FILENO, "/dev/null"))))
      goto done;
    if (ERROR((rc = proc_fd_new(proc, STDOUT_FILENO, "/dev/stdout"))))
      goto done;
    if (ERROR((rc = proc_fd_new(proc, STDERR_FILENO, "/dev/stderr"))))
      goto done;
    if (ERROR((rc = proc_fd_new(proc, XCACHE_FILENO, ""))))
      goto done;
  }

  // allocate space for the upcoming thread to avoid dealing with a messy ENOMEM
  // after fork
  if (ERROR((rc = LIST_RESERVE(&inf->threads, LIST_SIZE(&inf->threads) + 1))))
    goto done;

  {
    pid_t pid = fork();
    if (ERROR(pid < 0)) {
      rc = errno;
      goto done;
    }

    if (pid == 0) {
      inferior_exec(inf, cmd, spy);
      // unreachable
    }

    // discard the write end of this pipe so that our child’s closure (during
    // `execve`/`exit` in `inferior_exec`) will unblock reads on the other end
    (void)close(inf->exec_status[1]);

    thread.id = pid;
    thread.proc->id = pid;
  }

  DEBUG("waiting for the child (TID %ld) to SIGSTOP itself…", (long)thread.id);
  {
    int status;
    if (ERROR(waitpid(thread.id, &status, 0) < 0)) {
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
      thread_exit(&thread, rc);
      goto done;
    }
    if (ERROR(WIFSIGNALED(status))) {
      DEBUG("child died with signal %d", WTERMSIG(status));
      thread_exit(&thread, 128 + WTERMSIG(status));
      rc = ECHILD;
      goto done;
    }
    assert(WIFSTOPPED(status));
    if (ERROR(WSTOPSIG(status) != SIGSTOP)) {
      // FIXME: doesn’t this leave the child stuck and unknown to us?
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
    if (inf->mode == XC_EARLY_SECCOMP || inf->mode == XC_LATE_SECCOMP)
      opts |= PTRACE_O_TRACESECCOMP;
#endif
    if (ERROR(ptrace(PTRACE_SETOPTIONS, thread.id, NULL, opts) < 0)) {
      rc = errno;
      goto done;
    }
  }

  // resume the child
  if (inf->mode == XC_SYSCALL) {
    if (ERROR((rc = thread_syscall(thread))))
      goto done;
  } else {
    if (ERROR((rc = thread_cont(thread))))
      goto done;
  }

  if (ERROR((rc = LIST_PUSH_BACK(&inf->threads, thread))))
    goto done;
  thread = (thread_t){0};

done:
  thread_exit(&thread, EXIT_FAILURE);
  free(spy);

  return rc;
}
