#include <assert.h>
#include <stddef.h>
#include <unistd.h>
#include "woros.h"

void woros_i_am_reader(woros_t *woros) {

  assert(woros != NULL);
  assert(woros->base != NULL);
  assert(woros->size > 0);

  // the caller should not have previously called `woros_i_am_writer`, so the
  // read sides of this buffer should still be open
#ifndef NDEBUG
  for (size_t i = 0; i < woros->size; ++i)
    assert(woros->base[i].fds[0] > 0);
#endif

  // close all the write ends of the pipes we no longer need
  for (size_t i = 0; i < woros->size; ++i) {
    if (woros->base[i].fds[1] > 0) {
      (void)close(woros->base[i].fds[1]);
      woros->base[i].fds[1] = -1;
    }
  }
}
