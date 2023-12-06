#include "debug.h"
#include "path.h"
#include "proc_t.h"
#include "tee_t.h"
#include "trace_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcache/cmd.h>

int proc_save(proc_t *proc, const xc_cmd_t cmd, const char *trace_root) {

  assert(trace_root != NULL);

  char *path = NULL;
  int fd = 0;
  FILE *f = NULL;
  int rc = 0;

  // drain stdout and stderr
  if (ERROR((rc = tee_join(proc->t_out))))
    goto done;
  if (ERROR((rc = tee_join(proc->t_err))))
    goto done;

  // create a new trace file to save to
  path = path_join(trace_root, "XXXXXX.trace");
  if (ERROR(path == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  fd = mkostemps(path, strlen(".trace"), O_CLOEXEC);
  if (ERROR(fd < 0)) {
    rc = errno;
    goto done;
  }

  f = fdopen(fd, "w");
  if (ERROR(f == NULL)) {
    rc = errno;
    goto done;
  }
  fd = 0;

  // construct a trace object to write out
  const xc_trace_t trace = {
      .cmd = cmd, .inputs = proc->inputs, .n_inputs = proc->n_inputs};
  // TODO: outputs

  if (ERROR((rc = trace_save(trace, f))))
    goto done;

done:
  if (f != NULL)
    (void)fclose(f);
  if (fd > 0)
    (void)close(fd);
  free(path);

  return rc;
}
