#include "macros.h"
#include "trace.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

int trace_append_read(xc_trace_t *trace, read_file f) {

  assert(trace != NULL);
  assert(f.path != NULL);

  // expand the collection of read files
  size_t new_len = trace->read_len + 1;
  read_file *rs = realloc(trace->read, sizeof(trace->read[0]) * new_len);
  if (UNLIKELY(rs == NULL))
    return ENOMEM;
  trace->read = rs;
  trace->read_len = new_len;

  // append the new entry
  trace->read[trace->read_len - 1] = f;

  return 0;
}
