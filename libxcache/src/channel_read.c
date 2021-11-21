#include "channel.h"
#include "macros.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int channel_read(channel_t *channel, int *data) {

  assert(channel != NULL);
  assert(channel->out != NULL);
  assert(data != NULL);

  if (UNLIKELY(fread(data, sizeof(*data), 1, channel->out) != 1)) {
    if (feof(channel->out)) {
      *data = 0;
      return 0;
    }
    return ferror(channel->out);
  }

  return 0;
}
