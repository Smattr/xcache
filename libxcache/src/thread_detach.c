#include "debug.h"
#include "proc_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/ptrace.h>

int thread_detach(thread_t thread, int sig) {

  assert(thread.id > 0);

  if (ERROR(ptrace(PTRACE_DETACH, thread.id, NULL, sig) < 0)) {
    // if the thread disappeared when we were trying to detach it (can happen),
    // treat this as unsupported
    if (errno == ESRCH)
      return ECHILD;
    return errno;
  }

  return 0;
}
