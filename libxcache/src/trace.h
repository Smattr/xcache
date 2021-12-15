#pragma once

#include "fs_set.h"
#include "macros.h"
#include <xcache/trace.h>

struct xc_trace {

  /// files accessed
  fs_set_t io;

  /// status the process returned on exit
  int exit_status;
};

/// clean up and deallocate the members of a trace
INTERNAL void trace_deinit(xc_trace_t *trace);
