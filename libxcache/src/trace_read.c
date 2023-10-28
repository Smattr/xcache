#include "cbor.h"
#include "cmd_t.h"
#include "input_t.h"
#include "output_t.h"
#include "trace_t.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/trace.h>

int trace_read(xc_trace_t *trace, FILE *stream) {

  assert(trace != NULL);
  assert(stream != NULL);

  *trace = (xc_trace_t){0};
  xc_trace_t t = {0};
  int rc = 0;

  // check the leading tag confirms this is a serialised input
  {
    uint64_t tag = 0;
    if ((rc = cbor_read_u64_raw(stream, &tag, 0xc0)))
      goto done;
    // “trace”, remembering CBOR stores data big endian
    const char expected[sizeof(uint64_t)] = {'\0', '\0', '\0', 'e',
                                             'c',  'a',  'r',  't'};
    if (memcmp(&tag, &expected, sizeof(tag)) != 0) {
      rc = EPROTO;
      goto done;
    }
  }

  if ((rc = cmd_read(&t.cmd, stream)))
    goto done;

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

  {
    uint64_t n_outputs = 0;
    if ((rc = cbor_read_u64_raw(stream, &n_outputs, 0x80)))
      goto done;
    if (n_outputs > SIZE_MAX)
      return EOVERFLOW;

    if (n_outputs > 0) {
      t.outputs = calloc((size_t)n_outputs, sizeof(t.outputs[0]));
      if (t.outputs == NULL) {
        rc = ENOMEM;
        goto done;
      }
      t.n_outputs = (size_t)n_outputs;
    }
  }

  for (size_t i = 0; i < t.n_outputs; ++i) {
    if ((rc = output_read(&t.outputs[i], stream)))
      goto done;
  }

  *trace = t;
  t = (xc_trace_t){0};

done:
  xc_trace_free(&t);

  return rc;
}
