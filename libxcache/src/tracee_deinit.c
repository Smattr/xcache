#include "trace.h"
#include "tracee.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void tracee_deinit(tracee_t *tracee) {

  if (tracee == NULL)
    return;

  trace_deinit(&tracee->trace);

  if (tracee->err_path != NULL)
    (void)unlink(tracee->err_path);
  free(tracee->err_path);
  tracee->err_path = NULL;

  if (tracee->err_f != NULL)
    (void)fclose(tracee->err_f);
  tracee->err_f = NULL;

  if (tracee->out_path != NULL)
    (void)unlink(tracee->out_path);
  free(tracee->out_path);
  tracee->out_path = NULL;

  if (tracee->out_f != NULL)
    (void)fclose(tracee->out_f);
  tracee->out_f = NULL;

  for (size_t i = 0; i < sizeof(tracee->err) / sizeof(tracee->err[0]); ++i) {
    if (tracee->err[i] > 0)
      (void)close(tracee->err[i]);
    tracee->err[i] = 0;
  }

  for (size_t i = 0; i < sizeof(tracee->out) / sizeof(tracee->out[0]); ++i) {
    if (tracee->out[i] > 0)
      (void)close(tracee->out[i]);
    tracee->out[i] = 0;
  }

  while (tracee->fds != NULL) {
    open_file_t *of = tracee->fds;
    tracee->fds = of->next;
    free(of->path);
    free(of);
  }

  free(tracee->cwd);
  tracee->cwd = NULL;

  // TODO what to do if the tracee is still live (pid > 0)?

  tracee->proc = NULL;
}
