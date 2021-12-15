#include "copy_file.h"
#include "fs_set.h"
#include "macros.h"
#include "trace.h"
#include <errno.h>
#include <stdlib.h>
#include <xcache/trace.h>

int xc_trace_replay(const xc_trace_t *trace) {

  if (UNLIKELY(trace == NULL))
    return EINVAL;

  for (size_t i = 0; i < trace->io.size; ++i) {
    if (UNLIKELY(trace->io.base[i].path == NULL))
      return EINVAL;
    if (trace->io.base[i].written) {
      if (UNLIKELY(trace->io.base[i].content_path == NULL))
        return EINVAL;
    }
  }

  for (size_t i = 0; i < trace->io.size; ++i) {
    if (!trace->io.base[i].written)
      continue;
    const char *src = trace->io.base[i].content_path;
    const char *dst = trace->io.base[i].path;
    int rc = copy_file(src, dst);
    if (UNLIKELY(rc != 0))
      return rc;
  }

  exit(trace->exit_status);
}
