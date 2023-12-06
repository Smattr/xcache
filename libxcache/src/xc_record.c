#include "db_t.h"
#include "debug.h"
#include "event.h"
#include "proc_t.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <xcache/cmd.h>
#include <xcache/db.h>
#include <xcache/record.h>
#include <xcache/trace.h>

int xc_record(xc_db_t *db, xc_cmd_t cmd, unsigned mode) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(cmd.argc == 0))
    return EINVAL;

  if (ERROR(cmd.argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < cmd.argc; ++i) {
    if (ERROR(cmd.argv[i] == NULL))
      return EINVAL;
  }

  if (ERROR((mode & XC_MODE_AUTO) == 0))
    return EINVAL;

  char *trace_root = NULL;
  proc_t proc = {0};
  int rc = 0;

  // find a usable recording mode
  mode = xc_record_modes(mode);
  if (ERROR(mode == 0)) {
    rc = ENOSYS;
    goto done;
  }

  // derive the trace root directory
  if (ERROR((rc = db_trace_root(*db, cmd, &trace_root))))
    goto done;

  // create it if it does not exist
  if (mkdir(trace_root, 0755) < 0) {
    if (ERROR(errno != EEXIST)) {
      rc = errno;
      goto done;
    }
  }

  if (ERROR((rc = proc_new(&proc, mode, trace_root))))
    goto done;

  if (ERROR((rc = proc_start(&proc, cmd))))
    goto done;

  while (true) {
    int status;
    DEBUG("waiting on child %ld…", (long)proc.pid);
    if (ERROR(waitpid(proc.pid, &status, 0) < 0)) {
      rc = errno;
      goto done;
    }
    assert((WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(status) ||
            WIFCONTINUED(status)) &&
           "unknown waitpid status");
    assert(!WIFCONTINUED(status) &&
           "waitpid indicated SIGCONT when we did not request it");

    // we should not have received any of the events we did not ask for
    assert(!is_exit(status));
    assert(!is_vfork_done(status));

    // did the child exit?
    if (WIFEXITED(status)) {
      proc.pid = 0;
      const int exit_status = WEXITSTATUS(status);
      if (ERROR(exit_status != EXIT_SUCCESS)) {
        rc = ECHILD;
        goto done;
      }
      break;
    }

    // was the child killed by a signal?
    if (ERROR(WIFSIGNALED(status))) {
      DEBUG("pid %ld died with signal %d", (long)proc.pid, WTERMSIG(status));
      proc.pid = 0;
      errno = ECHILD;
      goto done;
    }

    if (is_fork(status)) {
      DEBUG("child forked");
      // The target called fork (or a cousin of). Unless I have missed
      // something in the ptrace docs, the only way to also trace forked
      // children is to set `PTRACE_O_FORK` and friends on the root process.
      // Unfortunately the result of this is that we get two events that tell
      // us the same thing: a `SIGTRAP` in the parent on fork (this case) and a
      // `SIGSTOP` in the child before execution (handled below). It is simpler
      // to just ignore the `SIGTRAP` in the parent and start tracking the child
      // when we receive its initial `SIGSTOP`.
      if (ERROR((rc = proc_cont(proc))))
        goto done;
      continue;
    }

    if (is_seccomp(status)) {
      assert((proc.mode == XC_EARLY_SECCOMP || proc.mode == XC_LATE_SECCOMP) &&
             "received a seccomp stop when we did not request it");
      rc = ENOTSUP; // TODO
      goto done;
    }

    if (is_syscall(status)) {
      if (proc.pending_sysexit) {
        if (ERROR((rc = sysexit(&proc))))
          goto done;
      } else {
        if (ERROR((rc = sysenter(&proc))))
          goto done;
      }
      proc.pending_sysexit = !proc.pending_sysexit;
      continue;
    }

    // we do not care about exec events
    if (is_exec(status)) {
      DEBUG("pid %ld, PTRACE_EVENT_EXEC", (long)proc.pid);
      if (proc.mode == XC_SYSCALL) {
        if (ERROR((rc = proc_syscall(proc))))
          goto done;
      } else {
        if (ERROR((rc = proc_cont(proc))))
          goto done;
      }
      continue;
    }

    {
      const int sig = WSTOPSIG(status);
      DEBUG("pid %ld, stopped by signal %d", (long)proc.pid, sig);
      if (ERROR((rc = proc_signal(proc, sig))))
        goto done;
    }
  }

  // save the result
  if (ERROR((rc = proc_save(&proc, cmd, trace_root))))
    goto done;

done:
  // if the child did something unsupported, let it run uninstrumented to
  // completion
  if (rc == ECHILD || rc == ESRCH)
    proc_detach(&proc);

  proc_free(proc);
  free(trace_root);

  return rc;
}
