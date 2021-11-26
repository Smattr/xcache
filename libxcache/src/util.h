#pragma once

#include "macros.h"
#include <errno.h>
#include <fcntl.h>

static inline int set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (UNLIKELY(fcntl(fd, F_SETFL, flags) != 0))
    return errno;
  return 0;
}
