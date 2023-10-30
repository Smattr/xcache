#include "debug.h"
#include "proc_t.h"
#include "trace_t.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/cmd.h>
#include <xcache/db.h>
#include <xcache/trace.h>

int xc_record(xc_db_t *db, xc_cmd_t cmd) {

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

  xc_trace_t *trace = NULL;
  proc_t proc = {0};
  int rc = 0;

  if (ERROR((rc = proc_new(&proc))))
    goto done;

  rc = ENOSYS;
  goto done;

done:
  proc_free(proc);
  xc_trace_free(trace);

  return rc;
}
