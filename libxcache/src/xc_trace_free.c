#include "input_t.h"
#include "list.h"
#include "output_t.h"
#include "trace_t.h"
#include <stddef.h>
#include <unistd.h>
#include <xcache/cmd.h>
#include <xcache/trace.h>

void xc_trace_free(xc_trace_t *trace) {

  if (trace == NULL)
    return;

  LIST_FREE(&trace->outputs, output_free);
  LIST_FREE(&trace->inputs, input_free);

  xc_cmd_free(trace->cmd);

  if (trace->root > 0)
    (void)close(trace->root);

  *trace = (xc_trace_t){0};
}
