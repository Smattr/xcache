#include "debug.h"
#include "list.h"
#include "trace_t.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/trace.h>

int xc_replay(const xc_trace_t *trace) {

  if (ERROR(trace == NULL))
    return EINVAL;

  int rc = 0;

  for (size_t i = 0; i < LIST_SIZE(&trace->outputs); ++i) {
    if ((rc = output_replay(*LIST_AT(&trace->outputs, i), trace)))
      goto done;
  }

done:
  return rc;
}
