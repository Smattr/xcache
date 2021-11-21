#include "channel.h"
#include <stddef.h>
#include <stdio.h>

void channel_close(channel_t *channel) {

  if (channel == NULL)
    return;

  if (channel->in != NULL) {
    (void)fclose(channel->in);
    channel->in = NULL;
  }

  if (channel->out != NULL) {
    (void)fclose(channel->out);
    channel->out = NULL;
  }
}
