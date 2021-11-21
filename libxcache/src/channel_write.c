#include "channel.h"
#include "macros.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int channel_write(channel_t *channel, int data) {

  assert(channel != NULL);
  assert(channel->in != NULL);

  if (UNLIKELY(fwrite(&data, sizeof(data), 1, channel->in) != 1))
    return ferror(channel->in);

  return 0;
}
