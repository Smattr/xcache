#include "../../common/proccall.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <xcache/cmd.h>

_Noreturn void proc_exec(const proc_t *proc, const xc_cmd_t cmd,
                         const char *spy) {

  assert(proc != NULL);
  assert(proc->t_out->pipe[1] > 0);
  assert(proc->t_err->pipe[1] > 0);
  assert(cmd.cwd != NULL);
  assert(cmd.argv != NULL);
  assert(cmd.argc > 0);
  for (size_t i = 0; i < cmd.argc; ++i)
    assert(cmd.argv[i] != NULL);
  assert(cmd.argv[cmd.argc] == NULL);
  assert(spy != NULL);

  int rc = 0;

  // dup /dev/null over stdin so tracing cannot depend on interactive input
  {
    int in = open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (in < 0) {
      rc = errno;
      goto fail;
    }
    if (dup2(in, STDIN_FILENO) < 0) {
      rc = errno;
      goto fail;
    }
  }

  // replace our stdout, stderr with pipes to the parent
  if (dup2(proc->t_out->pipe[1], STDOUT_FILENO) < 0) {
    rc = errno;
    goto fail;
  }
  if (dup2(proc->t_err->pipe[1], STDERR_FILENO) < 0) {
    rc = errno;
    goto fail;
  }

  // setup our bridge for out-of-band messages
  if (dup2(proc->proccall[1], XCACHE_FILENO) < 0) {
    rc = errno;
    goto fail;
  }

  // LeakSanitizer does not work under ptrace (as we will be), so disable it
  if (setenv("ASAN_OPTIONS", "detect_leaks=0", 1) < 0) {
    rc = errno;
    goto fail;
  }

  // setup our spy to be injected
  if (setenv("LD_PRELOAD", spy, 1) < 0) {
    rc = errno;
    goto fail;
  }

  // opt-in to being a ptrace tracee
  if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
    rc = errno;
    goto fail;
  }

  // give our parent an opportunity to attach to us
  if (raise(SIGSTOP) != 0) {
    rc = errno;
    goto fail;
  }

  // chdir _after_ the tracer attaches to let it naturally count this as a read

  if ((rc = xc_cmd_exec(cmd)))
    goto fail;

fail:
  assert(rc != 0 && "reached child failure without failing status");

  // replicate some common Linux behaviour
  if (rc == ENOENT || rc == ENOTDIR) {
    rc = 127;
  } else if (rc == EACCES || rc == ENOEXEC || rc == EPERM) {
    rc = 126;
  }

  exit(rc);
}
