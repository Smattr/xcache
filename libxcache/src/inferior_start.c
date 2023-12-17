#include "../../common/proccall.h"
#include "debug.h"
#include "find_me.h"
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
#include "inferior_t.h"

int inferior_start(inferior_t *inf, const xc_cmd_t cmd) {

  assert(inf != NULL);
  assert(inf->n_procs == 0 && "proc already started?");

  char *spy = NULL;
  proc_t proc = {0};
  int rc = 0;

  // allocate space for the process
  if (inf->n_procs == inf->c_procs) {
    size_t c = inf->c_procs == 0 ? 1 : inf->c_procs * 2;
    proc_t *ps = realloc(inf->procs, c * sizeof(inf->procs[0]));
    if (ERROR(ps == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    inf->procs = ps;
    inf->c_procs = c;
  }

  if (ERROR((rc = find_spy(&spy))))
    goto done;

  proc.cwd = strdup(cmd.cwd);
  if (ERROR(proc.cwd == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // we dup /dev/null over the child’s stdin
  if (ERROR((rc = proc_fd_new(&proc, STDIN_FILENO, "/dev/null"))))
    goto done;
  if (ERROR((rc = proc_fd_new(&proc, STDOUT_FILENO, "/dev/stdout"))))
    goto done;
  if (ERROR((rc = proc_fd_new(&proc, STDERR_FILENO, "/dev/stderr"))))
    goto done;
  if (ERROR((rc = proc_fd_new(&proc, XCACHE_FILENO, ""))))
    goto done;

  // allocate space for the child’s thread before forking, to avoid dealing with
  // a messy ENOMEM after fork
  proc.threads = calloc(1, sizeof(proc.threads[0]));
    if (ERROR(proc.threads == NULL)) {
      rc= ENOMEM;
      goto done;
    }
    proc.c_threads = 1;

  {
    pid_t pid = fork();
    if (ERROR(pid < 0)) {
      rc = errno;
      goto done;
    }

    if (pid == 0) {
      inferior_exec(inf, &proc, cmd, spy);
      // unreachable
    }

    proc.id = pid;

    assert(proc.n_threads < proc.c_threads);
    assert(proc.n_threads == 0);
    proc.threads[proc.n_threads].id = pid;
    ++proc.n_threads;
  }

  DEBUG("waiting for the child (pid %ld) to SIGSTOP itself…", (long)proc.id);
  {
    int status;
    if (ERROR(waitpid(proc.threads[0].id, &status, 0) < 0)) {
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
      proc.id = 0;
      --proc.n_threads;
      goto done;
    }
    if (ERROR(WIFSIGNALED(status))) {
      DEBUG("child died with signal %d", WTERMSIG(status));
      proc_exit(&proc, 128 + WTERMSIG(status));
      rc = ECHILD;
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
    if (inf->mode == XC_EARLY_SECCOMP || inf->mode == XC_LATE_SECCOMP)
      opts |= PTRACE_O_TRACESECCOMP;
#endif
    if (ERROR(ptrace(PTRACE_SETOPTIONS, proc.threads[0].id, NULL, opts) < 0)) {
      rc = errno;
      goto done;
    }
  }

  // resume the child
  if (inf->mode == XC_SYSCALL) {
    if (ERROR((rc = thread_syscall(proc.threads[0]))))
      goto done;
  } else {
    if (ERROR((rc = thread_cont(proc.threads[0]))))
      goto done;
  }

  assert(inf->c_procs > inf->n_procs);
  inf->procs[inf->n_procs] = proc;
  ++inf->n_procs;
  proc = (proc_t){0};

done:
  proc_end(&proc);
  free(spy);

  return rc;
}
