#include "debug.h"
#include "proc_t.h"
#include "trace_t.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/cmd.h>
#include <xcache/trace.h>

int xc_trace_record(xc_db_t *db, xc_cmd_t cmd, xc_trace_t **trace) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(cmd.argc == 0))
    return EINVAL;

  if (ERROR(cmd.argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < cmd.argc; ++i) {
    if (ERROR(cmd.argv[i] == NULL))
      return EINVAL;
  }

  if (ERROR(trace == NULL))
    return EINVAL;

  *trace = NULL;
  xc_trace_t *t = NULL;
  proc_t proc = {0};
  int rc = 0;

  if (ERROR((rc = proc_new(&proc))))
    goto done;

  rc = ENOSYS;
  goto done;

  *trace = t;
  t = NULL;

done:
  proc_free(proc);
  xc_trace_free(t);

  return rc;
}
