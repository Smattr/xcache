#include "input_t.h"
#include "trace_t.h"
#include <stdlib.h>
#include <unistd.h>
#include <xcache/trace.h>

void xc_trace_free(xc_trace_t *trace) {

  if (trace == NULL)
    return;

  for (size_t i = 0; i < trace->n_inputs; ++i)
    input_free(trace->inputs[i]);
  free(trace->inputs);

  if (trace->root > 0)
    (void)close(trace->root);

  *trace = (xc_trace_t){0};
}
