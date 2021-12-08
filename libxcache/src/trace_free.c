#include "trace.h"
#include <stdlib.h>
#include <xcache/trace.h>

void xc_trace_free(xc_trace_t *trace) {

  if (trace == NULL)
    return;

  trace_deinit(trace);

  free(trace);
}
