#pragma once

#include "macros.h"
#include <stdio.h>

/// an in-memory pipe for communication between a parent and child
typedef struct {
  FILE *in;
  FILE *out;
} channel_t;

/// create a new channel
///
/// \param channel [out] Created channel on success
/// \return 0 on success or an errno on failure
INTERNAL int channel_open(channel_t *channel);

/// write a value to a channel
///
/// \param channel Channel to operate on
/// \param data Value to write
/// \return 0 on success or an errno on failure
INTERNAL int channel_write(channel_t *channel, int data);

/// read a value from a channel
///
/// If the other party closes the channel, it is considered that they have
/// implicitly sent `0` to the channel. That is, reading the channel will return
/// a zero value.
///
/// \param channel Channel to operate on
/// \param data [out] Value read from the channel
/// \return 0 on success or an errno on failure
INTERNAL int channel_read(channel_t *channel, int *data);

/// destroy a channel
///
/// \param channel Channel to close
INTERNAL void channel_close(channel_t *channel);
