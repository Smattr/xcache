#include "debug.h"
#include "event.h"
#include "proc_t.h"
#include "trace_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <xcache/cmd.h>
#include <xcache/db.h>
#include <xcache/record.h>
#include <xcache/trace.h>

int xc_record(xc_db_t *db, xc_cmd_t cmd) {

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

  xc_trace_t *trace = NULL;
  proc_t proc = {0};
  int rc = 0;

  if (ERROR((rc = proc_new(&proc))))
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
    assert(!is_exec(status));
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
      proc.pid = 0;
      DEBUG("child died with signal %d", WTERMSIG(status));
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
      rc = ENOTSUP; // TODO
      goto done;
    }

    if (is_syscall(status)) {
      rc = ENOTSUP; // TODO
      goto done;
    }

    DEBUG("child stopped by signal %d", WSTOPSIG(status));
    if (ERROR((rc = proc_cont(proc))))
      goto done;
  }

  rc = ENOSYS;

done:
  proc_free(proc);
  xc_trace_free(trace);

  return rc;
}
