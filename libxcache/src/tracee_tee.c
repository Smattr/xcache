#include "debug.h"
#include "macros.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

static __attribute__((unused)) bool is_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return !!(flags & O_NONBLOCK);
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

void *tracee_tee(void *arg) {

  assert(arg != NULL);

  int rc = 0;
  tracee_t *tracee = arg;

  while (tracee->out[0] > 0 && tracee->err[0] > 0) {

    // poll for events on the stdout, stderr pipes
    DEBUG("polling the child...");
    assert(is_nonblocking(tracee->out[0]));
    assert(is_nonblocking(tracee->err[0]));
    struct pollfd fds[] = {
        {.fd = tracee->out[0] > 0 ? tracee->out[0] : -1, .events = POLLIN},
        {.fd = tracee->err[0] > 0 ? tracee->err[0] : -1, .events = POLLIN},
    };
    nfds_t nfds = sizeof(fds) / sizeof(fds[0]);
    if (UNLIKELY(poll(fds, nfds, -1) == -1)) {
      rc = errno;
      goto done;
    }

    for (size_t i = 0; i < sizeof(fds) / sizeof(fds[0]); ++i) {

      // skip disabled entries
      if (fds[i].fd == -1)
        continue;

      if (fds[i].revents & POLLERR)
        DEBUG("error condition on fd %d", fds[i].fd);

      // is there data available to read?
      if (fds[i].revents & POLLIN) {
        if (fds[i].fd == tracee->out[0]) {
          DEBUG("child has stdout data");
          rc = drain(STDOUT_FILENO, fds[i].fd);
        } else {
          assert(fds[i].fd == tracee->err[0]);
          DEBUG("child has stderr data");
          rc = drain(STDERR_FILENO, fds[i].fd);
        }
        if (UNLIKELY(rc != 0))
          goto done;
      }

      // did the tracee close their end of the pipe?
      if (fds[i].revents & POLLHUP) {
        if (fds[i].fd == tracee->out[0]) {
          DEBUG("child closed stdout");
          (void)close(tracee->out[0]);
          tracee->out[0] = 0;
        } else {
          assert(fds[i].fd == tracee->err[0]);
          DEBUG("child closed stderr");
          (void)close(tracee->err[0]);
          tracee->err[0] = 0;
        }
      }
    }
  }

done:
  return (void *)(intptr_t)rc;
}
