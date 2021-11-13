#include "macros.h"
#include "trace.h"
#include <stdbool.h>
#include <stddef.h>
#include <xcache/hash.h>
#include <xcache/trace.h>

bool xc_trace_is_valid(const xc_trace_t *trace) {

  if (UNLIKELY(trace == NULL))
    return false;

  if (UNLIKELY(trace->read_len > 0 && trace->read == NULL))
    return false;

  for (size_t i = 0; i < trace->read_len; ++i) {
    if (UNLIKELY(trace->read[i].path == NULL))
      return false;
  }

  // does the current hash of each file match those seen by the trace?
  for (size_t i = 0; i < trace->read_len; ++i) {

    xc_hash_t hash;
    int rc = xc_hash_file(&hash, trace->read[i].path);
    if (rc != 0)
      return false;

    if (trace->read[i].hash != hash)
      return false;
  }

  return true;
}
