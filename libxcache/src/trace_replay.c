#include "copy_file.h"
#include "macros.h"
#include "trace.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcache/trace.h>

int xc_trace_replay(const xc_trace_t *trace) {

  if (UNLIKELY(trace == NULL))
    return EINVAL;

  if (UNLIKELY(trace->written_len > 0 && trace->written == NULL))
    return EINVAL;

  for (size_t i = 0; i < trace->written_len; ++i) {
    if (trace->written[i].path == NULL)
      return EINVAL;
    if (trace->written[i].content_path == NULL)
      return EINVAL;
  }

  for (size_t i = 0; i < trace->written_len; ++i) {
    const char *src = trace->written[i].content_path;
    const char *dst = trace->written[i].path;
    int rc = copy_file(src, dst);
    if (UNLIKELY(rc != 0))
      return rc;
  }

  exit(trace->exit_status);
}
