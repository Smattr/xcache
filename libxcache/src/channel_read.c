#include "channel.h"
#include "macros.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int channel_read(channel_t *channel, int *data) {

  assert(channel != NULL);
  assert(channel->in != NULL);
  assert(data != NULL);

  if (UNLIKELY(fread(data, sizeof(*data), 1, channel->in) != 1)) {
    if (feof(channel->in)) {
      *data = 0;
      return 0;
    }
    return ferror(channel->in);
  }

  return 0;
}
