#include "channel.h"
#include "debug.h"
#include "macros.h"
#include "trace.h"
#include "tracee.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
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

static int drain(int to, int from) {
  while (true) {
    char buffer[BUFSIZ];
    ssize_t r = read(from, buffer, sizeof(buffer));
    if (r < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return 0;
      return errno;
    }
    if (r == 0)
      return 0;

    size_t offset = 0;
    while (r > 0) {
      ssize_t w = write(to, &buffer[offset], (size_t)r);
      if (w < 0) {
        if (errno == EINTR)
          continue;
        return errno;
      }
      offset += (size_t)w;
      r -= (size_t)w;
    }
  }
  return 0;
}

static __attribute__((unused)) bool is_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return !!(flags & O_NONBLOCK);
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

    // poll the child for a relevant event
    DEBUG("polling the child...");
    assert(is_nonblocking(tracee->out[0]));
    assert(is_nonblocking(tracee->err[0]));
    assert(is_nonblocking(tracee->pidfd));
    struct pollfd fds[] = {
        {.fd = tracee->out[0], .events = POLLIN},
        {.fd = tracee->err[0], .events = POLLIN},
        {.fd = tracee->pidfd, .events = POLLIN},
    };
    nfds_t nfds = sizeof(fds) / sizeof(fds[0]);
    if (UNLIKELY(poll(fds, nfds, -1) == -1)) {
      rc = errno;
      goto done;
    }

    for (size_t i = 0; i < sizeof(fds) / sizeof(fds[0]); ++i) {

      // skip if nothing was available to read from this entry
      if (!(fds[i].revents & POLLIN))
        continue;

      // is this an entry indicating stdout data available?
      if (fds[i].fd == tracee->out[0]) {
        DEBUG("child has stdout data");
        rc = drain(STDOUT_FILENO, tracee->out[0]);
        if (UNLIKELY(rc != 0))
          goto done;
      }

      // is this an entry indicating stderr data available?
      if (fds[i].fd == tracee->err[0]) {
        DEBUG("child has stderr data");
        rc = drain(STDERR_FILENO, tracee->err[0]);
        if (UNLIKELY(rc != 0))
          goto done;
      }

      // is this an entry indicating a child PID event?
      if (fds[i].fd == tracee->pidfd) {
        DEBUG("child has a PID event");

        siginfo_t status;
        static const int options = WEXITED | WSTOPPED | WNOHANG;
        if (UNLIKELY(waitid(P_PID, tracee->pid, &status, options) == -1)) {
          rc = errno;
          goto done;
        }

        // was this an exit?
        if (status.si_code == CLD_EXITED) {
          trace->exit_status = status.si_status;
          goto done; // success
        }

        assert(status.si_code == CLD_TRAPPED && "TODO");

        // was this a seccomp event?
        if (status.si_status >> 8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))) {

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

        // TODO
      }
    }
  }

done:
  return rc;
}
