#include "channel.h"
#include "tracee.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void tracee_deinit(tracee_t *tracee) {

  if (tracee == NULL)
    return;

  tracee->proc = NULL;

  // TODO what to do if the tracee is still live (pid > 0)?

  for (size_t i = 0; i < sizeof(tracee->out) / sizeof(tracee->out[0]); ++i) {
    if (tracee->out[i] > 0)
      (void)close(tracee->out[i]);
    tracee->out[i] = 0;
  }

  for (size_t i = 0; i < sizeof(tracee->err) / sizeof(tracee->err[0]); ++i) {
    if (tracee->err[i] > 0)
      (void)close(tracee->err[i]);
    tracee->err[i] = 0;
  }

  if (tracee->out_content != NULL)
    (void)fclose(tracee->out_content);
  tracee->out_content = NULL;

  free(tracee->out_base);
  tracee->out_base = NULL;
  tracee->out_len = 0;

  if (tracee->err_content != NULL)
    (void)fclose(tracee->err_content);
  tracee->err_content = NULL;

  free(tracee->err_base);
  tracee->err_base = NULL;
  tracee->err_len = 0;

  channel_close(&tracee->msg);
}
