#pragma once

#include "input_t.h"
#include <xcache/trace.h>

struct xc_trace {
  int root; ///< handle to open trace-containing directory

  input_t *inputs; ///< inputs that were read by the tracee
  size_t n_inputs; ///< number of items in `inputs`

  // TODO: outputs
};
