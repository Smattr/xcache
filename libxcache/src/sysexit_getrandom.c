#include "debug.h"
#include "inferior_t.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>

int sysexit_getrandom(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  (void)inf;

  int rc = 0;

  DEBUG("TID %ld, getrandom(…)", (long)thread->id);

  // if the spy has not told us to ignore these calls, give up
  if (ERROR(!thread->ignoring_rng)) {
    rc = ECHILD;
    goto done;
  }

done:
  return rc;
}
