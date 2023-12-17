#include "proc_t.h"
#include <sys/ptrace.h>
#include "debug.h"
#include <assert.h>
#include <stddef.h>
#include <errno.h>

int thread_detach(thread_t thread, int sig) {

  assert(thread.id > 0);

  int rc = 0;

  if (ERROR(ptrace(PTRACE_DETACH, thread.id, NULL, sig) < 0)) {
    // if the thread disappeared when we were trying to detach it (can happen),
    // treat this as unsupported
    if (errno == ESRCH)
      return ECHILD;
    rc = errno;
    goto done;
  }

done:
  return rc;
}
