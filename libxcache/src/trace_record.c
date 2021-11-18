#include "macros.h"
#include "proc.h"
#include "trace.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

static int child(const xc_proc_t *proc, int in, int out, int err, int msg) {

  // replace our streams with pipes to the parent
  if (UNLIKELY(dup2(in, STDIN_FILENO) == -1))
    return errno;
  if (UNLIKELY(dup2(out, STDOUT_FILENO) == -1))
    return errno;
  if (UNLIKELY(dup2(err, STDERR_FILENO) == -1))
    return errno;

  // set close-on-exec on our pipe back to the parent, so they will learn we
  // correctly execed if their read on the pipe fails
  int flags = fcntl(msg, F_GETFD);
  flags |= FD_CLOEXEC;
  if (UNLIKELY(fcntl(msg, F_SETFD, flags) != 0))
    return errno;

  // TODO: opt-in to ptrace

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
  int in[] = {-1, -1};
  int out[] = {-1, -1};
  int err[] = {-1, -1};
  int msg[] = {-1, -1};

  xc_trace_t *t = calloc(1, sizeof(*t));
  if (UNLIKELY(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // setup pipes for stdin, stdout, stderr
  if (pipe(in) != 0 || pipe(out) != 0 || pipe(err) != 0) {
    rc = errno;
    goto done;
  }

  // setup a pipe for the child to signal exec failure to the parent
  if (pipe(msg) != 0) {
    rc = errno;
    goto done;
  }

  pid_t pid = fork();
  if (UNLIKELY(pid == -1)) {
    rc = errno;
    goto done;
  }

  // are we the child?
  if (pid == 0) {

    // close the descriptors we do not need
    (void)close(msg[0]);
    (void)close(err[0]);
    (void)close(out[0]);
    (void)close(in[1]);

    int r = child(proc, in[0], out[1], err[1], msg[1]);

    // we failed, so signal this to the parent
    {
      size_t size = sizeof(r);
      do {
        size_t offset = sizeof(r) - size;
        ssize_t w = write(msg[1], (char *)&r + offset, size);
        if (UNLIKELY(w < 0)) {
          // error, but we now have no way of signalling this, so give up
          break;
        }
        size -= (size_t)w;
      } while (size > 0);
    }

    exit(EXIT_FAILURE);
  }

  // close the descriptors we do not need
  (void)close(msg[1]);
  msg[1] = -1;
  (void)close(err[1]);
  err[1] = -1;
  (void)close(out[1]);
  out[1] = -1;
  (void)close(in[0]);
  in[0] = -1;

  // wait for a signal of failure from the child
  {
    size_t size = sizeof(rc);
    do {
      size_t offset = sizeof(rc) - size;
      ssize_t r = read(msg[0], (char *)&rc + offset, size);
      if (r == 0) {
        // the child execed successfully
        break;
      }
      if (r < 0) {
        if (errno == EINTR) {
          // suppress and try again
          r = 0;
        } else {
          rc = errno;
          goto done;
        }
      }
      size -= (size_t)r;
    } while (size > 0);
  }

  // did the child tell us it failed?
  if (rc != 0)
    goto done;

  // we no longer need the message pipe
  (void)close(msg[0]);
  msg[0] = -1;

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
  if (msg[0] != -1)
    (void)close(msg[0]);
  if (msg[1] != -1)
    (void)close(msg[1]);
  if (err[0] != -1)
    (void)close(err[0]);
  if (err[1] != -1)
    (void)close(err[1]);
  if (out[0] != -1)
    (void)close(out[0]);
  if (out[1] != -1)
    (void)close(out[1]);
  if (in[0] != -1)
    (void)close(in[0]);
  if (in[1] != -1)
    (void)close(in[1]);
  if (UNLIKELY(rc != 0))
    free(t);

  return rc;
}
