#include "trace.h"
#include <assert.h>
#include <stddef.h>
#include <xcache/trace.h>

int xc_trace_exit_status(const xc_trace_t *trace) {

  assert(trace != NULL);

  return trace->exit_status;
}
