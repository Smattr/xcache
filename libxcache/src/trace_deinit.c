#include "trace.h"
#include <assert.h>
#include <stdlib.h>
#include <xcache/trace.h>

void trace_deinit(xc_trace_t *trace) {

  assert(trace != NULL);

  for (size_t i = 0; i < trace->written_len; ++i) {
    free(trace->written[i].path);
    trace->written[i].path = NULL;
    free(trace->written[i].content_path);
    trace->written[i].content_path = 0;
  }

  free(trace->written);
  trace->written = NULL;
  trace->written_len = 0;

  for (size_t i = 0; i < trace->read_len; ++i) {
    free(trace->read[i].path);
    trace->read[i].path = NULL;
  }

  free(trace->read);
  trace->read = NULL;
  trace->read_len = 0;
}
