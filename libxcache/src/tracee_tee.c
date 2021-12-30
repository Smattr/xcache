#include "debug.h"
#include "macros.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static __attribute__((unused)) bool is_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return !!(flags & O_NONBLOCK);
}

/// return code for something that can have either hard or soft failures
typedef struct {
  int soft_rc;
  int hard_rc;
} rc_t;

static rc_t drain(int to, FILE *to_buffer, int from) {
  while (true) {
    char buffer[BUFSIZ];

    // read data from our pipe shared with the child
    ssize_t r = read(from, buffer, sizeof(buffer));
    if (r < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return (rc_t){0};
      return (rc_t){.hard_rc = errno};
    }
    if (r == 0)
      return (rc_t){0};

    size_t size = (size_t)r;

    // write the data to our own stdout/stderr
    for (size_t offset = 0; offset < size;) {
      ssize_t w = write(to, &buffer[offset], size - offset);
      if (w < 0) {
        if (errno == EINTR)
          continue;
        return (rc_t){.hard_rc = errno};
      }
      offset += (size_t)w;
    }

    // write the data to our in-memory buffer for saving later
    if (LIKELY(to_buffer != NULL)) {
      size_t written = fwrite(buffer, sizeof(buffer[0]), size, to_buffer);
      if (ERROR(written != size))
        return (rc_t){.soft_rc = ENOMEM};
    }
  }
  UNREACHABLE();
}

static void discard_buffers(tracee_t *tracee) {

  // discard and deallocate stdout sink
  if (tracee->out_f != NULL)
    (void)fclose(tracee->out_f);
  tracee->out_f = NULL;
  if (tracee->out_path != NULL)
    (void)unlink(tracee->out_path);
  free(tracee->out_path);
  tracee->out_path = NULL;

  // discard and deallocate stderr sink
  if (tracee->err_f != NULL)
    (void)fclose(tracee->err_f);
  tracee->err_f = NULL;
  if (tracee->err_path != NULL)
    (void)unlink(tracee->err_path);
  free(tracee->err_path);
  tracee->err_path = NULL;
}

void *tracee_tee(void *arg) {

  assert(arg != NULL);

  int rc = 0;
  tracee_t *tracee = arg;

  // an error that should not stop us processing the child’s streams, but will
  // eventually result in overall failure
  int soft_rc = 0;

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
    if (ERROR(poll(fds, nfds, -1) == -1)) {
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
        rc_t r = {0};
        if (fds[i].fd == tracee->out[0]) {
          DEBUG("child has stdout data");
          r = drain(STDOUT_FILENO, tracee->out_f, fds[i].fd);
        } else {
          assert(fds[i].fd == tracee->err[0]);
          DEBUG("child has stderr data");
          r = drain(STDERR_FILENO, tracee->err_f, fds[i].fd);
        }

        if (ERROR(r.soft_rc != 0)) {
          discard_buffers(tracee);
          if (soft_rc == 0)
            soft_rc = r.soft_rc;
        } else if (ERROR(r.hard_rc != 0)) {
          rc = r.hard_rc;
          goto done;
        }
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
  if (rc == 0 && UNLIKELY(soft_rc != 0))
    rc = soft_rc;

  return (void *)(intptr_t)rc;
}
