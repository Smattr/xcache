#include "../../common/compiler.h"
#include "debug.h"
#include "input_t.h"
#include "trace_t.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <xcache/trace.h>

bool xc_trace_is_valid(const xc_trace_t *trace) {

  if (trace == NULL)
    return false;

  // are all the inputs in the same state as they were originally perceived?
  assert(trace->n_inputs == 0 || trace->inputs != NULL);
  for (size_t i = 0; i < trace->n_inputs; ++i) {
    if (!input_is_valid(trace->inputs[i])) {
      if (UNLIKELY(xc_debug != NULL)) {
        char *input_str = input_to_str(trace->inputs[i]);

        DEBUG("input %zu (%s) no longer valid", i,
              input_str == NULL ? "<oom>" : input_str);
        free(input_str);
      }
      return false;
    }
  }

  return true;
}
