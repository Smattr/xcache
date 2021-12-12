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

static int init(tracee_t *tracee) {

  int rc = 0;

  // wait for the child to SIGSTOP itself
  DEBUG("waiting for the child to SIGSTOP itself...");
  {
    int status;
    if (UNLIKELY(waitpid(tracee->pid, &status, 0) == -1)) {
      rc = errno;
      DEBUG("failed to wait on SIGSTOP from the child: %d", rc);
      goto done;
    }
    if (UNLIKELY(WIFEXITED(status))) {
      rc = WEXITSTATUS(status);
      DEBUG("child prematurely exited with %d", rc);
      goto done;
    }
    else if (LIKELY(WIFSTOPPED(status))) {
      if (UNLIKELY(WSTOPSIG(status) != SIGSTOP)) {
        rc = status;
        DEBUG("non-SIGSTOP signal from child: %d", WSTOPSIG(status));
        goto done;
      }
    } else {
      rc = status;
      DEBUG("unrecognized exit from child: %d", status);
      goto done;
    }
  }

  // set our tracer preferences
  DEBUG("setting ptrace preferences...");
  {
    static const int opts = PTRACE_O_TRACESECCOMP | PTRACE_O_TRACECLONE |
                            PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK;
    if (UNLIKELY(ptrace(PTRACE_SETOPTIONS, tracee->pid, NULL, opts) != 0)) {
      rc = errno;
      goto done;
    }
  }

  // resume the child
  rc = tracee_resume(tracee);
  if (UNLIKELY(rc != 0))
    goto done;

done:
  return rc;
}

/// possible events the tracer can see
typedef enum {
  EV_FORK,    // `fork` or a cousin thereof
  EV_SECCOMP, // seccomp (mid-syscall) stop
  EV_SYSCALL, // ptrace syscall end stop
  EV_SIGNAL,  // signal delivery stop
} event_t;

static event_t get_event(int status) {

  assert(WIFSTOPPED(status));

  switch (status >> 8) {

  case SIGTRAP | (PTRACE_EVENT_FORK << 8):
  case SIGTRAP | (PTRACE_EVENT_VFORK << 8):
  case SIGTRAP | (PTRACE_EVENT_CLONE << 8):
    return EV_FORK;

  case SIGTRAP | (PTRACE_EVENT_SECCOMP << 8):
    return EV_SECCOMP;

  case SIGTRAP:
    return EV_SYSCALL;

  // ptrace events corresponding to options that we never set and hence should
  // never receive
  case SIGTRAP | (PTRACE_EVENT_EXEC << 8):
  case SIGTRAP | (PTRACE_EVENT_EXIT << 8):
  case SIGTRAP | (PTRACE_EVENT_VFORK_DONE << 8):
    DEBUG("unexpected ptrace event %d", status);
    UNREACHABLE();

    // otherwise, this was a signal
  default:
    break;
  }

  return EV_SIGNAL;
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

      switch (get_event(status)) {

      case EV_FORK:
        DEBUG("target called fork");
        // The target called fork (or a cousin of). Unless I have missed
        // something in the ptrace docs, the only way to also trace forked
        // children is to set PTRACE_O_FORK and friends on the root process.
        // Unfortunately the result of this is that we get two events that tell
        // us the same thing: a SIGTRAP in the parent on fork (this case) and a
        // SIGSTOP in the child before execution (handled below). It is simpler
        // to just ignore the SIGTRAP in the parent and start tracking the child
        // when we receive its initial SIGSTOP.
        rc = tracee_resume(tracee);
        if (UNLIKELY(rc != 0))
          goto done;

        continue;

      case EV_SECCOMP:
        DEBUG("saw seccomp stop");

        rc = syscall_middle(tracee);
        if (UNLIKELY(rc != 0))
          goto done;

        continue;

      // vanilla SIGTRAP
      case EV_SYSCALL:
        DEBUG("saw normal SIGTRAP stop");
        break;
      }

      rc = syscall_end(tracee);
      if (UNLIKELY(rc != 0))
        goto done;

      // resume the child
      rc = tracee_resume(tracee);
      if (UNLIKELY(rc != 0))
        goto done;

      continue;
    }

    // was the child stopped by a signal?
    if (WIFSTOPPED(status)) {
      DEBUG("child stopped by signal %d", WSTOPSIG(status));

      // resume the child
      rc = tracee_resume(tracee);
      if (UNLIKELY(rc != 0))
        goto done;

      continue;
    }

    DEBUG("unhandled child stop");
    UNREACHABLE();
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
