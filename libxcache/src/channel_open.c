#include "channel.h"
#include "macros.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int channel_open(channel_t *channel) {

  assert(channel != NULL);

  memset(channel, 0, sizeof(*channel));

  int rc = 0;

  // create a new in-memory pipe
  int fds[2] = {-1, -1};
  if (UNLIKELY(pipe2(fds, O_CLOEXEC) < 0)) {
    rc = errno;
    goto done;
  }

  // convert the read end to a FILE handle
  FILE *in = fdopen(fds[0], "r");
  if (UNLIKELY(in == NULL)) {
    rc = errno;
    goto done;
  }
  channel->in = in;
  fds[0] = -1;

  // convert the write end to a FILE handle
  FILE *out = fdopen(fds[1], "w");
  if (UNLIKELY(out == NULL)) {
    rc = errno;
    goto done;
  }
  channel->out = out;
  fds[1] = -1;

done:
  if (UNLIKELY(rc != 0)) {
    channel_close(channel);
    if (fds[0] != -1)
      (void)close(fds[0]);
    if (fds[1] != -1)
      (void)close(fds[1]);
  }

  return rc;
}
