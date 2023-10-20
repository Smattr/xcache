#include "cbor.h"
#include "input_t.h"
#include "trace_t.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcache/trace.h>

int trace_read(xc_trace_t *trace, FILE *stream) {

  assert(trace != NULL);
  assert(stream != NULL);

  *trace = (xc_trace_t){0};
  xc_trace_t t = {0};
  int rc = 0;

  {
    uint64_t n_inputs = 0;
    if ((rc = cbor_read_u64_raw(stream, &n_inputs, 0x80)))
      goto done;
    if (n_inputs > SIZE_MAX)
      return EOVERFLOW;

    if (n_inputs > 0) {
      t.inputs = calloc((size_t)n_inputs, sizeof(t.inputs[0]));
      if (t.inputs == NULL) {
        rc = ENOMEM;
        goto done;
      }
      t.n_inputs = (size_t)n_inputs;
    }
  }

  for (size_t i = 0; i < t.n_inputs; ++i) {
    if ((rc = input_read(&t.inputs[i], stream)))
      goto done;
  }

  *trace = t;
  t = (xc_trace_t){0};

done:
  xc_trace_free(&t);

  return rc;
}
