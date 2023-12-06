#include "debug.h"
#include "tee_t.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

int tee_join(tee_t *tee) {

  assert(tee != NULL);

  int rc = 0;

  // close the write end of the pipe to EOF the child
  if (tee->pipe[1] > 0)
    (void)close(tee->pipe[1]);
  tee->pipe[1] = 0;

  // wait for the child to finish
  if (tee->initialised) {
    do {
      void *retval;
      if (ERROR((rc = pthread_join(tee->forwarder, &retval))))
        break;
      if (ERROR((rc = (int)(intptr_t)retval)))
        break;
    } while (0);
  }
  tee->initialised = false;

  if (tee->copy_fd > 0)
    (void)close(tee->copy_fd);
  tee->copy_fd = 0;

  // leave `copy_path` as-is

  if (tee->pipe[0] > 0)
    (void)close(tee->pipe[0]);
  tee->pipe[0] = 0;

  return rc;
}
