#include "debug.h"
#include "macros.h"
#include "proc.h"
#include "trace.h"
#include "tracee.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <xcache/db_t.h>
#include <xcache/trace.h>

int xc_trace_record(xc_trace_t **trace, const xc_proc_t *proc, xc_db_t *db) {

  if (ERROR(trace == NULL))
    return EINVAL;

  if (ERROR(proc == NULL))
    return EINVAL;

  if (ERROR(proc->argc == 0))
    return EINVAL;

  if (ERROR(proc->argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < proc->argc; ++i) {
    if (ERROR(proc->argv[i] == NULL))
      return EINVAL;
  }

  if (ERROR(db == NULL))
    return EINVAL;

  int rc = -1;
  tracee_t tracee = {0};
  xc_trace_t *t = NULL;

  rc = tracee_init(&tracee, proc, db);
  if (ERROR(rc != 0))
    goto done;

  t = calloc(1, sizeof(*t));
  if (ERROR(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  tracee.pid = fork();
  if (ERROR(tracee.pid == -1)) {
    rc = errno;
    goto done;
  }

  // are we the child (tracee)?
  if (tracee.pid == 0) {
    tracee_exec(&tracee);
    UNREACHABLE();
  }

  // we are the parent (tracer)

  // close the descriptors we do not need
  (void)close(tracee.err[1]);
  tracee.err[1] = 0;
  (void)close(tracee.out[1]);
  tracee.out[1] = 0;

  rc = tracee_monitor(&tracee);
  if (ERROR(rc != 0 && rc != ENOTSUP))
    goto done;

  // success
  *t = tracee.trace;
  memset(&tracee.trace, 0, sizeof(tracee.trace));
  *trace = t;
  t = NULL;

done:
  free(t);
  tracee_deinit(&tracee);

  return rc;
}
