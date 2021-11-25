#include "channel.h"
#include "debug.h"
#include "macros.h"
#include "proc.h"
#include "trace.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

static int child(const xc_proc_t *proc, int out, int err, channel_t *msg) {

  assert(proc != NULL);
  assert(out >= 0);
  assert(err >= 0);
  assert(msg != NULL);

  // dup /dev/null over stdin so tracing cannot depend on interactive input
  {
    int in = open("/dev/null", O_RDONLY);
    if (UNLIKELY(in == -1))
      return errno;
    if (UNLIKELY(dup2(in, STDIN_FILENO) == -1)) {
      int r = errno;
      (void)close(in);
      return r;
    }
  }

  // replace our streams with pipes to the parent
  if (UNLIKELY(dup2(out, STDOUT_FILENO) == -1))
    return errno;
  if (UNLIKELY(dup2(err, STDERR_FILENO) == -1))
    return errno;

  // opt-in to being a ptrace tracee
  if (UNLIKELY(ptrace(PTRACE_TRACEME, 0, NULL, NULL) != 0))
    return errno;

  // signal to out parent that we passed phase 1 of our setup
  (void)channel_write(msg, 0);

  // give our parent an opportunity to attach to us
  (void)raise(SIGSTOP);

  // TODO: install seccomp filter

  return xc_proc_exec(proc);
}

int xc_trace_record(xc_trace_t **trace, const xc_proc_t *proc) {

  if (UNLIKELY(trace == NULL))
    return EINVAL;

  if (UNLIKELY(proc == NULL))
    return EINVAL;

  if (UNLIKELY(proc->argc == 0))
    return EINVAL;

  if (UNLIKELY(proc->argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < proc->argc; ++i) {
    if (UNLIKELY(proc->argv[i] == NULL))
      return EINVAL;
  }

  int rc = 0;
  int out[] = {-1, -1};
  int err[] = {-1, -1};
  channel_t msg = {0};

  xc_trace_t *t = calloc(1, sizeof(*t));
  if (UNLIKELY(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // setup pipes for stdout, stderr
  if (pipe(out) != 0 || pipe(err) != 0) {
    rc = errno;
    goto done;
  }

  // setup a channel for the child to signal exec failure to the parent
  rc = channel_open(&msg);
  if (UNLIKELY(rc != 0))
    goto done;

  pid_t pid = fork();
  if (UNLIKELY(pid == -1)) {
    rc = errno;
    goto done;
  }

  // are we the child?
  if (pid == 0) {

    // close the descriptors we do not need
    (void)close(err[0]);
    (void)close(out[0]);

    int r = child(proc, out[1], err[1], &msg);

    // we failed, so signal this to the parent
    (void)channel_write(&msg, r);

    exit(EXIT_FAILURE);
  }

  // close the descriptors we do not need
  (void)close(err[1]);
  err[1] = -1;
  (void)close(out[1]);
  out[1] = -1;

  // wait for a signal of failure from the child
  DEBUG("waiting on signal from child...\n");
  {
    int r = channel_read(&msg, &rc);
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
    if (UNLIKELY(waitpid(pid, &status, 0) == -1)) {
      rc = errno;
      DEBUG("failed to wait on SIGSTOP from the child: %d\n", rc);
      goto done;
    }
  }

  // set our tracer preferences
  DEBUG("setting ptrace preferences...\n");
  {
    static const int opts = PTRACE_O_TRACESECCOMP;
    if (UNLIKELY(ptrace(PTRACE_SETOPTIONS, pid, NULL, 0, opts) != 0)) {
      rc = errno;
      goto done;
    }
  }

  // resume the child
  DEBUG("resuming the child...\n");
  if (UNLIKELY(ptrace(PTRACE_CONT, pid, NULL, NULL) != 0)) {
    rc = errno;
    DEBUG("failed to continue the child: %d\n", rc);
    goto done;
  }

  // wait for a signal of failure from the child in case they fail exec
  DEBUG("waiting on signal from child...\n");
  {
    int r = channel_read(&msg, &rc);
    DEBUG("channel read of %d, rc = %d from the child\n", r, rc);
    if (UNLIKELY(r != 0)) {
      rc = r;
      goto done;
    }
    if (UNLIKELY(rc != 0))
      goto done;
  }

  // we no longer need the message channel
  channel_close(&msg);

  // TODO: monitor the child

  // wait for the child to finish
  int status;
  if (UNLIKELY(waitpid(pid, &status, 0) == -1)) {
    rc = errno;
    goto done;
  }

  // decode its exit status
  if (WIFEXITED(status)) {
    t->exit_status = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    t->exit_status = 128 + WTERMSIG(status);
  } else {
    t->exit_status = -1;
  }

  *trace = t;

done:
  channel_close(&msg);
  if (err[0] != -1)
    (void)close(err[0]);
  if (err[1] != -1)
    (void)close(err[1]);
  if (out[0] != -1)
    (void)close(out[0]);
  if (out[1] != -1)
    (void)close(out[1]);
  if (UNLIKELY(rc != 0))
    free(t);

  return rc;
}
