#include "debug.h"
#include "list.h"
#include "trace_t.h"
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <xcache/trace.h>

int xc_replay(const xc_trace_t *trace) {

  if (ERROR(trace == NULL))
    return EINVAL;

  int rc = 0;

  // set a umask that allows us to create files with any mode
  const mode_t old_umask = umask(0);

  for (size_t i = 0; i < LIST_SIZE(&trace->outputs); ++i) {
    if (ERROR((rc = output_replay(*LIST_AT(&trace->outputs, i), trace))))
      goto done;
  }

done:
  // restore previous umask
  (void)umask(old_umask);

  return rc;
}
