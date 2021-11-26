#include "macros.h"
#include "proc.h"
#include "trace.h"
#include "tracee.h"
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <xcache/trace.h>

int xc_trace_record(xc_trace_t **trace, const xc_proc_t *proc) {

  if (UNLIKELY(trace == NULL))
    return EINVAL;

  if (UNLIKELY(proc == NULL))
    return EINVAL;

  if (UNLIKELY(proc->argc == 0))
    return EINVAL;

  if (UNLIKELY(proc->argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < proc->argc; ++i) {
    if (UNLIKELY(proc->argv[i] == NULL))
      return EINVAL;
  }

  int rc = 0;
  tracee_t tracee = {0};
  xc_trace_t *t = NULL;

  rc = tracee_init(&tracee, proc);
  if (UNLIKELY(rc != 0))
    goto done;

  t = calloc(1, sizeof(*t));
  if (UNLIKELY(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  tracee.pid = fork();
  if (UNLIKELY(tracee.pid == -1)) {
    rc = errno;
    goto done;
  }

  // are we the child (tracee)?
  if (tracee.pid == 0)
    tracee_exec(&tracee);

  // we are the parent (tracer)

  // close the descriptors we do not need
  (void)close(tracee.err[1]);
  tracee.err[1] = 0;
  (void)close(tracee.out[1]);
  tracee.out[1] = 0;

  rc = tracee_monitor(t, &tracee);

done:
  tracee_deinit(&tracee);
  if (UNLIKELY(rc != 0))
    free(t);

  return rc;
}
