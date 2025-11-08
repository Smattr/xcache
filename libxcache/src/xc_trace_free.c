#include "input_t.h"
#include "list.h"
#include "output_t.h"
#include "trace_t.h"
#include <stdlib.h>
#include <unistd.h>
#include <xcache/cmd.h>
#include <xcache/trace.h>

void xc_trace_free(xc_trace_t *trace) {

  if (trace == NULL)
    return;

  for (size_t i = 0; i < trace->n_outputs; ++i)
    output_free(trace->outputs[i]);
  free(trace->outputs);

  for (size_t i = 0; i < LIST_SIZE(&trace->inputs); ++i)
    input_free(*LIST_AT(&trace->inputs, i));
  LIST_FREE(&trace->inputs);

  xc_cmd_free(trace->cmd);

  if (trace->root > 0)
    (void)close(trace->root);

  *trace = (xc_trace_t){0};
}
