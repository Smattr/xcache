#include "debug.h"
#include "trace_t.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/trace.h>

int xc_trace_replay(const xc_trace_t *trace) {

  if (ERROR(trace == NULL))
    return EINVAL;

  int rc = 0;

  for (size_t i = 0; i < trace->n_outputs; ++i) {
    if ((rc = output_replay(trace->outputs[i], trace)))
      goto done;
  }

done:
  return rc;
}
