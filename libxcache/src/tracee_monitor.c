#include "channel.h"
#include "debug.h"
#include "macros.h"
#include "trace.h"
#include "tracee.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>

static int init(tracee_t *tracee) {

  int rc = 0;

  // wait for a signal of failure from the child
  DEBUG("waiting on signal from child...\n");
  {
    int r = channel_read(&tracee->msg, &rc);
    DEBUG("channel read of %d, rc = %d from the child\n", r, rc);
    if (UNLIKELY(r != 0)) {
      rc = r;
      goto done;
    }
    if (UNLIKELY(rc != 0))
      goto done;
  }

  // wait for the child to SIGSTOP itself
  DEBUG("waiting for the child to SIGSTOP itself...\n");
  {
    int status;
    if (UNLIKELY(waitpid(tracee->pid, &status, 0) == -1)) {
      rc = errno;
      DEBUG("failed to wait on SIGSTOP from the child: %d\n", rc);
      goto done;
    }
  }

  // set our tracer preferences
  DEBUG("setting ptrace preferences...\n");
  {
    static const int opts = PTRACE_O_TRACESECCOMP;
    if (UNLIKELY(ptrace(PTRACE_SETOPTIONS, tracee->pid, NULL, 0, opts) != 0)) {
      rc = errno;
      goto done;
    }
  }

  // resume the child
  DEBUG("resuming the child...\n");
  if (UNLIKELY(ptrace(PTRACE_CONT, tracee->pid, NULL, NULL) != 0)) {
    rc = errno;
    DEBUG("failed to continue the child: %d\n", rc);
    goto done;
  }

  // wait for a signal of failure from the child in case they fail exec
  DEBUG("waiting on signal from child...\n");
  {
    int r = channel_read(&tracee->msg, &rc);
    DEBUG("channel read of %d, rc = %d from the child\n", r, rc);
    if (UNLIKELY(r != 0)) {
      rc = r;
      goto done;
    }
    if (UNLIKELY(rc != 0))
      goto done;
  }

  // we no longer need the message channel
  channel_close(&tracee->msg);

  // get a descriptor for the child we can use in `select` calls
  {
    int pidfd = pidfd_open(tracee->pid, 0);
    if (UNLIKELY(pidfd == -1)) {
      rc = errno;
      goto done;
    }
    tracee->pidfd = pidfd;
  }
  rc = set_nonblock(tracee->pidfd);
  if (UNLIKELY(rc != 0))
    goto done;

done:
  return rc;
}

int tracee_monitor(xc_trace_t *trace, tracee_t *tracee) {

  assert(trace != NULL);
  assert(tracee != NULL);
  assert(tracee->pid > 0 && "tracee not started");

  int rc = 0;

  rc = init(tracee);
  if (UNLIKELY(rc != 0))
    goto done;

  // monitor the child
  while (true) {

    int status;
    if (UNLIKELY(waitpid(tracee->pid, &status, 0) == -1)) {
      rc = errno;
      goto done;
    }

    // was this a seccomp event?
    if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))) {

      // retrieve the syscall number
      static const size_t RAX_OFFSET =
          offsetof(struct user, regs) +
          offsetof(struct user_regs_struct, orig_rax);
      long nr = ptrace(PTRACE_PEEKUSER, tracee->pid, RAX_OFFSET, NULL);
      DEBUG("saw syscall %ld from the child\n", nr);

      // resume the child
      DEBUG("resuming the child...\n");
      if (UNLIKELY(ptrace(PTRACE_CONT, tracee->pid, NULL, NULL) != 0)) {
        rc = errno;
        DEBUG("failed to continue the child: %d\n", rc);
        goto done;
      }

      continue;
    }

    // if not, this was an exit

    // decode its exit status
    if (WIFEXITED(status)) {
      trace->exit_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      trace->exit_status = 128 + WTERMSIG(status);
    } else {
      trace->exit_status = -1;
    }

    break;
  }

done:
  return rc;
}
