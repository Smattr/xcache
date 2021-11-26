#include "channel.h"
#include "tracee.h"
#include <stddef.h>
#include <unistd.h>

void tracee_deinit(tracee_t *tracee) {

  if (tracee == NULL)
    return;

  tracee->proc = NULL;

  // TODO what to do if the tracee is still live (pid > 0)?

  if (tracee->pidfd > 0) {
    (void)close(tracee->pidfd);
    tracee->pidfd = 0;
  }

  for (size_t i = 0; i < sizeof(tracee->out) / sizeof(tracee->out[0]); ++i) {
    if (tracee->out[i] > 0) {
      (void)close(tracee->out[i]);
      tracee->out[i] = 0;
    }
  }

  for (size_t i = 0; i < sizeof(tracee->err) / sizeof(tracee->err[0]); ++i) {
    if (tracee->err[i] > 0) {
      (void)close(tracee->err[i]);
      tracee->err[i] = 0;
    }
  }

  channel_close(&tracee->msg);
}
