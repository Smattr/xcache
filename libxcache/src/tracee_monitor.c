#include "channel.h"
#include "debug.h"
#include "macros.h"
#include "trace.h"
#include "tracee.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

// XXX: there is no Glibc wrapper for this
static int pidfd_open(pid_t pid) { return syscall(__NR_pidfd_open, pid, 0); }

static int init(tracee_t *tracee) {

  int rc = 0;

  // wait for a signal of failure from the child
  DEBUG("waiting on signal from child...");
  {
    int r = channel_read(&tracee->msg, &rc);
    DEBUG("channel read of %d, rc = %d from the child", r, rc);
    if (UNLIKELY(r != 0)) {
      rc = r;
      goto done;
    }
    if (UNLIKELY(rc != 0))
      goto done;
  }

  // wait for the child to SIGSTOP itself
  DEBUG("waiting for the child to SIGSTOP itself...");
  {
    int status;
    if (UNLIKELY(waitpid(tracee->pid, &status, 0) == -1)) {
      rc = errno;
      DEBUG("failed to wait on SIGSTOP from the child: %d", rc);
      goto done;
    }
  }

  // set our tracer preferences
  DEBUG("setting ptrace preferences...");
  {
    static const int opts = PTRACE_O_TRACESECCOMP;
    if (UNLIKELY(ptrace(PTRACE_SETOPTIONS, tracee->pid, NULL, 0, opts) != 0)) {
      rc = errno;
      goto done;
    }
  }

  // resume the child
  DEBUG("resuming the child...");
  if (UNLIKELY(ptrace(PTRACE_CONT, tracee->pid, NULL, NULL) != 0)) {
    rc = errno;
    DEBUG("failed to continue the child: %d", rc);
    goto done;
  }

  // wait for a signal of failure from the child in case they fail exec
  DEBUG("waiting on signal from child...");
  {
    int r = channel_read(&tracee->msg, &rc);
    DEBUG("channel read of %d, rc = %d from the child", r, rc);
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
    int pidfd = pidfd_open(tracee->pid);
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
  bool tee_created = false;
  pthread_t tee;

  rc = init(tracee);
  if (UNLIKELY(rc != 0))
    goto done;

  // create a background thread to handle the tracee’s output streams
  rc = pthread_create(&tee, NULL, tracee_tee, tracee);
  if (UNLIKELY(rc != 0))
    goto done;
  tee_created = true;

  // monitor the child
  while (true) {

    DEBUG("waiting on the child...");
    int status;
    if (UNLIKELY(waitpid(tracee->pid, &status, 0) == -1)) {
      rc = errno;
      goto done;
    }

    // was this an exit?
    if (WIFEXITED(status)) {
      DEBUG("child exited");
      trace->exit_status = WEXITSTATUS(status);
      goto done; // success
    }

    // was this a tracing event?
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {

      switch (status >> 8) {

      // `fork` alike
      case SIGTRAP | (PTRACE_EVENT_FORK << 8):
      case SIGTRAP | (PTRACE_EVENT_VFORK << 8):
      case SIGTRAP | (PTRACE_EVENT_CLONE << 8):
        break;

      // `exec alike
      case SIGTRAP | (PTRACE_EVENT_EXEC << 8):
        break;

      // seccomp event
      case SIGTRAP | (PTRACE_EVENT_SECCOMP << 8):
        break;

      default:
        DEBUG("warning: unhandled SIGTRAP stop %d", status);
      }

      // retrieve the syscall number
      static const size_t RAX_OFFSET =
          offsetof(struct user, regs) +
          offsetof(struct user_regs_struct, orig_rax);
      long nr = ptrace(PTRACE_PEEKUSER, tracee->pid, RAX_OFFSET, NULL);
      DEBUG("saw syscall %ld from the child", nr);

      // resume the child
      DEBUG("resuming the child...");
      if (UNLIKELY(ptrace(PTRACE_CONT, tracee->pid, NULL, NULL) != 0)) {
        rc = errno;
        DEBUG("failed to continue the child: %d", rc);
        goto done;
      }

      continue;
    }

    // was the child stopped by a signal?
    if (WIFSTOPPED(status)) {
      DEBUG("child stopped by signal %d", WSTOPSIG(status));

      // resume the child
      DEBUG("resuming the child...");
      if (UNLIKELY(ptrace(PTRACE_CONT, tracee->pid, NULL, NULL) != 0)) {
        rc = errno;
        DEBUG("failed to continue the child: %d", rc);
        goto done;
      }
    }

    // TODO
  }

done:
  // FIXME: handle the tee thread being blocked on a poll
  if (tee_created) {
    void *r;
    int err = pthread_join(tee, &r);
    if (err != 0) {
      if (rc == 0)
        rc = err;
    } else if (rc == 0) {
      rc = (int)(intptr_t)r;
    }
  }

  return rc;
}
