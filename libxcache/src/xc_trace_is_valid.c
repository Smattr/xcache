#include "../../common/compiler.h"
#include "debug.h"
#include "input_t.h"
#include "list.h"
#include "trace_t.h"
#include <stdbool.h>
#include <stdlib.h>
#include <xcache/trace.h>

bool xc_trace_is_valid(const xc_trace_t *trace) {

  if (trace == NULL)
    return false;

  // are all the inputs in the same state as they were originally perceived?
  for (size_t i = 0; i < LIST_SIZE(&trace->inputs); ++i) {
    const input_t input = *LIST_AT(&trace->inputs, i);
    if (!input_is_valid(input)) {
      if (UNLIKELY(xc_debug != NULL)) {
        char *input_str = input_to_str(input);

        DEBUG("input %zu (%s) no longer valid", i,
              input_str == NULL ? "<oom>" : input_str);
        free(input_str);
      }
      return false;
    }
  }

  return true;
}
