#include "fs_set.h"
#include "trace.h"
#include <assert.h>
#include <stddef.h>
#include <xcache/trace.h>

void trace_deinit(xc_trace_t *trace) {

  assert(trace != NULL);

  fs_set_free(&trace->io);
}
