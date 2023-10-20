#include "input_t.h"
#include "trace_t.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <xcache/trace.h>

bool xc_trace_is_valid(const xc_trace_t *trace) {

  if (trace == NULL)
    return false;

  // are all the inputs in the same state as they were originally perceived?
  assert(trace->n_inputs == 0 || trace->inputs != NULL);
  for (size_t i = 0; i < trace->n_inputs; ++i) {
    if (!input_is_valid(trace->inputs[i]))
      return false;
  }

  return true;
}
