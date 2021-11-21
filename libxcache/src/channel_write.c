#include "channel.h"
#include "macros.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int channel_write(channel_t *channel, int data) {

  assert(channel != NULL);
  assert(channel->out != NULL);

  if (UNLIKELY(fwrite(&data, sizeof(data), 1, channel->out) != 1))
    return ferror(channel->out);

  // ensure the message gets immediately propagated to the received
  (void)fflush(channel->out);

  return 0;
}
